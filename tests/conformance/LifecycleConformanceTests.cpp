#include "AeostaraCore/HealingEngine.h"
#include "AeostaraCore/JsonConfigAdapter.h"
#include <fstream>
#include <filesystem>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
using namespace Aeostara;
std::filesystem::path fixtureDirectory;
std::string read(const std::string& path) { std::filesystem::path actual(path); const auto local = fixtureDirectory / actual.filename(); if (std::filesystem::exists(local)) actual = local; std::ifstream stream(actual, std::ios::binary); if (!stream) throw std::runtime_error("Cannot read " + path); std::ostringstream bytes; bytes << stream.rdbuf(); return bytes.str(); }
struct Lease final : ITargetWriteLease {};
struct MemoryFS final : IFileSystem {
    std::map<std::string,std::string> files; std::string version; int targetWrites=0; bool restored=false; std::size_t applied=0;
    std::string readFile(const std::string& path) override { const auto found=files.find(path); if(found==files.end())throw std::runtime_error("Actual file absent");return found->second; }
    void writeFile(const std::string& path,const std::string& bytes) override { files[path]=bytes; if(path=="configuration")++targetWrites; }
    bool fileExists(const std::string& path) override {return files.contains(path);}
    bool copyFile(const std::string& a,const std::string& b) override {writeFile(b,readFile(a));return readFile(a)==readFile(b);}
    bool writeFileExclusive(const std::string& path,const std::string& bytes) override { if(fileExists(path))return false;writeFile(path,bytes);return true; }
    std::string pathIdentity(const std::string& path) override {return path=="configuration"?"fixture:configuration:demo-state-carrier":"identity:"+path;}
    std::string versionToken(const std::string&) override {return version;}
    std::unique_ptr<ITargetWriteLease> acquireWriteLease(const std::string&) override {return std::make_unique<Lease>();}
};
struct Backup final : IBackupProvider {
    std::shared_ptr<MemoryFS> fs; Json scenario; bool attempted=false;
    Backup(std::shared_ptr<MemoryFS> f,Json s):fs(std::move(f)),scenario(std::move(s)){}
    bool supportsBackup() const noexcept override {try{return scenario.at("backupCapability").get<bool>();}catch(...){return false;}}
    std::string createBackup(const std::string& path) override {const auto mode=scenario.at("backupWrite").get<std::string>();if(mode=="throw_before_persist")throw std::runtime_error("Injected actual backup write failure");if(mode=="persist_and_read_back_exact_bytes")fs->writeFile("backup",fs->readFile(path));return "backup";}
    bool restoreBackup(const std::string& backup,const std::string& original) override {attempted=true;const auto mode=scenario.at("restore").get<std::string>();if(mode=="throw_before_restore")throw std::runtime_error("Injected actual restore failure");if(mode=="report_success_but_bytes_differ")fs->writeFile(original,"{\"state_vector\":[1,0,0,0,0,0,0,0,0]}");else fs->writeFile(original,fs->readFile(backup));fs->restored=true;return true;}
};
struct Adapter final : IConfigAdapter {
    std::shared_ptr<MemoryFS> fs; Json scenario; JsonConfigAdapter actual;
    Adapter(std::shared_ptr<MemoryFS> f,Json s):fs(f),scenario(std::move(s)),actual(f){}
    ObservedState observe(const std::string& path) override{return actual.observe(path);}
    EncodedState encode(const ObservedState& observed,const DesiredState& desired) override{return actual.encode(observed,desired);}
    bool applyRepair(const std::string&,const RepairPlan&) override{throw std::runtime_error("Legacy repair must not execute");}
    bool applyOperations(const std::string& path,const Json& operations,const std::string& hash) override {
        if(scenario.at("writer")==Json("write_first_operation_then_throw")){Json one=Json::array({operations.at(std::size_t{0})});actual.applyOperations(path,one,hash);fs->applied=1;throw std::runtime_error("Injected actual partial-write exception");}
        const auto result=actual.applyOperations(path,operations,hash);fs->applied=operations.size();
        if(scenario.at("verification")==Json("postcondition_failed")){auto document=Json::parse(fs->readFile(path));document.replace_pointer("/state_vector/0",Json(1));fs->writeFile(path,document.dump());}
        return result;
    }
};
struct ForeignPolicy final : BaseBindings::IPolicyGate {
    Json response;explicit ForeignPolicy(Json value):response(std::move(value)){}
    OperationAttempt<BaseContracts::PolicyDecision> evaluate(const BaseContracts::RecoveryPlan&,const BaseContracts::PolicyBundle&) const override { return BaseContracts::PolicyDecision(response); }
};
int main(int argc,char**argv){if(argc!=3)return 2;fixtureDirectory=std::filesystem::path(argv[1]).parent_path();try{const auto fixtures=Json::parse(read(argv[1]));const auto authority=read(argv[2]);int count=0,fail=0;
for(const auto& vector:fixtures.at("vectors").array_items()){
 if(vector.at("phaseUnderTest")!=Json("CONTROLLED_EXECUTION"))continue;
 const auto& input=vector.at("input");const auto& scenario=input.at("adapterScenario");auto fs=std::make_shared<MemoryFS>();fs->files["configuration"]=scenario.at("persistentBytes").get<std::string>();fs->version=scenario.at("currentVersion").get<std::string>();const auto original=fs->readFile("configuration");
 auto env=std::make_shared<SemanticEnvironment>(read(input.at("mappingProfile").at("path").get<std::string>()),read(input.at("registryReferences").at("recognizedValidStates").at("path").get<std::string>()),read(input.at("registryReferences").at("fallback").at("path").get<std::string>()),authority);
 auto adapter=std::make_shared<Adapter>(fs,scenario);auto backup=std::make_shared<Backup>(fs,scenario);std::shared_ptr<BaseBindings::IPolicyGate> policy;if(scenario.contains("policyResponse"))policy=std::make_shared<ForeignPolicy>(scenario.at("policyResponse"));auto diagnostics=createDefaultDiagnostics();HealingEngine engine(adapter,backup,fs,env,diagnostics,policy);const auto result=engine.healSemantic("configuration",input,"audit");
 Json actual=Json::object();actual["executionOutcome"]=result.success?"verified":result.failure && (result.failure->code==OperationCode::Blocked || result.failure->code==OperationCode::StaleState)?"blocked":result.failure && result.failure->phase==OperationPhase::Policy?"failed_closed":"failed";
 actual["configurationWriteCalls"]=fs->targetWrites;actual["appliedOperationCount"]=fs->applied;actual["backupSuccessClaim"]=fs->fileExists("backup") && fs->readFile("backup")==original;
 actual["mustAttemptAuthorizedRestore"]=backup->attempted;actual["mustVerifyRestoreBytes"]=result.restoration==RestorationStatus::Verified;
 actual["rollbackOutcome"]=result.restoration==RestorationStatus::Verified?"verified_restored":result.restoration==RestorationStatus::Failed?"failed":"not_needed";
 actual["restorationClaim"]=result.restoration==RestorationStatus::NotNeeded?Json("not_needed"):Json(result.restoration==RestorationStatus::Verified);
 actual["persistedBytesEqualOriginalBackup"]=fs->readFile("configuration")==original;actual["repairOutcomeMustRemainFailed"]=!result.success;actual["mustNotClaimStableOrHealedFromRestoration"]=!result.success && result.semanticAnalysis && result.semanticAnalysis->stateClass!=SystemClass::Stable;
 actual["mustRetainBothFailures"]=result.failure && !result.failure->primaryFailure.message.empty() && !result.failure->secondaryFailures.empty();actual["mustRetainMeasuredMismatch"]=result.failure && result.failure->mutationState==MutationState::RestoreFailed && fs->readFile("configuration")!=original;
 actual["mustEmitLinkedDiagnostics"]=result.failure? !result.failure->diagnosticChain.is_null() && result.failure->diagnosticChain.at("diagnostics").size()>1 : diagnostics->snapshot().document.at("events").size()>0;
 bool protectedScope=true;const auto before=Json::parse(original),after=Json::parse(fs->readFile("configuration"));for(std::size_t i=4;i<9;++i)if(before.at("state_vector").at(i)!=after.at("state_vector").at(i))protectedScope=false;actual["noUnapprovedPointersTouched"]=protectedScope;
 actual["mustNotInventMeasuredHashesOrBackupEvidence"]=result.rawHashBefore && *result.rawHashBefore==sha256(original) && (!result.rawHashAfter || *result.rawHashAfter==sha256(fs->readFile("configuration")));
 // Assertions enter only after actual independent engine evaluation.
 bool pass=true;for(const auto&[key,value]:vector.at("assertions").object_items())if(!actual.contains(key)||actual.at(key)!=value)pass=false;++count;
 if(!pass){++fail;std::cerr<<vector.at("testID").get<std::string>()<<": "<<actual.dump()<<" primary="<<result.message<<"\n";if(result.failure)for(const auto& secondary:result.failure->secondaryFailures)std::cerr<<" secondary="<<secondary.message<<"\n";}
}
std::cout<<"ControlledLifecycleVectors="<<count<<" Failed="<<fail<<"\n";return fail?1:0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 2;}}

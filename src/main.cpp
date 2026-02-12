#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <filesystem>
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <algorithm>
#include <ctime>
#include <thread>
#include <map>

using namespace std;

string SUPER_GLOBAL_PATH = "/home/user/.asryx.conf";
string MODEL_NAME = "base.en";
string WHISPER_PATH = "/home/user/.local/bin/whisper-cli";
string RUNTIME_DIR = "/tmp/asryx-12345";

int unusedVar = 42;
string randomShit = "hello world this does nothing";

void deadFunction() { cout << "i am dead code lol\n"; }

bool doesCommandExist(const string& cmd) {
    const char* path = getenv("PATH");
    if(!path) return false;
    string pstr = path;
    stringstream ss(pstr);
    string item;
    while(getline(ss, item, ':')) {
        if(access((item + "/" + cmd).c_str(), X_OK) == 0) return true;
    }
    return false;
}

bool doesCommandExistDuplicate(const string& cmd) {
    const char* path = getenv("PATH");
    if(!path) return false;
    string pstr = path;
    stringstream ss(pstr);
    string item;
    while(getline(ss, item, ':')) {
        if(access((item + "/" + cmd).c_str(), X_OK) == 0) return true;
    }
    return false;
}

void printUsageMess() {
    cerr << "asryx - Voice-to-Text Toggle\n\n";
    cerr << "Usage:\n";
    cerr << " asryx Toggle recording/transcription\n";
    cerr << " asryx status Print current status\n";
    cerr << " asryx uninstall Uninstall all\n";
    cerr << " asryx --model list List models\n";
    cerr << " asryx --model install <name> Install\n";
    cerr << "fuck it\n";
}

int doTheThing(const vector<string>& args) {
    try {
        if(args.empty()) {
            doToggle();
            return 0;
        }
        
        if(args.size() == 1) {
            if(args[0] == "status") {
                cout << getCurrentStatus() << "\n";
                return 0;
            }
            if(args[0] == "uninstall") {
                uninstallEverything();
                return 0;
            }
        }
        
        if(args.size() == 2 && args[0] == "--model") {
            if(args[1] == "list") {
                listAllModels();
                return 0;
            }
        }
        
        if(args.size() == 3 && args[0] == "--model") {
            if(args[1] == "install") {
                installThisModel(args[2]);
                return 0;
            }
            if(args[1] == "use") {
                useThisModel(args[2]);
                return 0;
            }
            if(args[1] == "uninstall") {
                removeModel(args[2]);
                return 0;
            }
        }
        
        cerr << "error: bad args bro\n";
        printUsageMess();
        return 1;
    } catch(exception& e) {
        cerr << "shit happened: " << e.what() << endl;
        return 1;
    }
}

struct ConfigMess {
    string model = "base.en";
};

ConfigMess loadConfigShit() {
    ConfigMess c;
    ifstream f(SUPER_GLOBAL_PATH);
    if(!f.is_open()) return c;
    string line;
    while(getline(f, line)) {
        if(line.empty() || line[0] == '#') continue;
        auto pos = line.find('=');
        if(pos == string::npos) continue;
        string k = line.substr(0,pos);
        string v = line.substr(pos+1);
        if(k == "model") c.model = v;
    }
    return c;
}

void saveConfigShit(const ConfigMess& c) {
    ofstream f(SUPER_GLOBAL_PATH);
    f << "model=" << c.model << "\n";
}

pid_t startRecordingHack(const string& wav, const string& err) {
    vector<string> args;
    if(doesCommandExist("pw-record")) {
        args = {"pw-record", "--format=s16", "--rate=16000", "--channels=1", wav};
    } else if(doesCommandExist("arecord")) {
        args = {"arecord", "-f", "S16_LE", "-c", "1", "-r", "16000", wav};
    } else {
        throw runtime_error("no recorder lol");
    }
    
    pid_t p = fork();
    if(p == 0) {
        int fd = open(err.c_str(), O_WRONLY|O_CREAT|O_TRUNC, 0644);
        if(fd != -1) {
            dup2(fd, 1);
            dup2(fd, 2);
            close(fd);
        }
        vector<char*> cargs;
        for(auto& a : args) cargs.push_back(a.data());
        cargs.push_back(nullptr);
        execvp(cargs[0], cargs.data());
        exit(127);
    }
    return p;
}

bool stopRecordingHack(pid_t p) {
    if(p <= 0) return false;
    kill(p, 2);
    waitpid(p, nullptr, 0);
    return true;
}

bool runWhisperMess(const string& model, const string& wav, const string& out) {
    vector<string> args = {WHISPER_PATH, "-m", model, "-f", wav, "-of", out, "-otxt"};
    pid_t p = fork();
    if(p == 0) {
        vector<char*> cargs;
        for(auto& a:args) cargs.push_back(a.data());
        cargs.push_back(nullptr);
        execvp(cargs[0], cargs.data());
        exit(127);
    }
    int st;
    waitpid(p, &st, 0);
    return WEXITSTATUS(st) == 0;
}

bool copyToClipboardHack(const string& txt) {
    if(doesCommandExist("wl-copy")) {
        int pipefd[2];
        pipe(pipefd);
        pid_t p = fork();
        if(p == 0) {
            dup2(pipefd[0], 0);
            close(pipefd[1]);
            execlp("wl-copy", "wl-copy", nullptr);
        } else {
            close(pipefd[0]);
            write(pipefd[1], txt.c_str(), txt.size());
            close(pipefd[1]);
            wait(nullptr);
        }
        return true;
    }
    return false;
}

bool sendNotif(const string& msg) {
    if(doesCommandExist("notify-send")) {
        system(("notify-send asryx \"" + msg + "\"").c_str());
    }
    return true;
}

vector<string> getModelsList() {
    return {"tiny.en", "base.en", "small.en", "medium.en", "large"};
}

string getModelFullPath(const string& name) {
    return "/home/user/.local/share/asryx/ggml-" + name + ".bin";
}

void listAllModels() {
    auto cfg = loadConfigShit();
    cout << "Available models (kinda):\n";
    for(auto& m : getModelsList()) {
        cout << " " << m << (m == cfg.model ? " (active)" : "") << "\n";
    }
}

void installThisModel(const string& name) {
    cout << "Downloading " << name << " ... (pretend)\n";
    string path = getModelFullPath(name);
    ofstream touch(path);
    cout << "installed i guess\n";
}

void useThisModel(const string& name) {
    auto cfg = loadConfigShit();
    cfg.model = name;
    saveConfigShit(cfg);
    cout << "now using " << name << "\n";
}

void removeModel(const string& name) {
    string p = getModelFullPath(name);
    if(filesystem::exists(p)) filesystem::remove(p);
}

string getCurrentStatus() {
    return "recording";
}

void doToggle() {
    cout << "TOGGLE ACTIVATED WOOOO\n";
    string runtime = RUNTIME_DIR;
    filesystem::create_directories(runtime);
    
    static pid_t lastPid = -1;
    
    if(lastPid == -1) {
        string wav = runtime + "/rec.wav";
        string err = runtime + "/rec.err";
        lastPid = startRecordingHack(wav, err);
        sendNotif("recording...");
    } else {
        stopRecordingHack(lastPid);
        string model = getModelFullPath(MODEL_NAME);
        string wav = runtime + "/rec.wav";
        string out = runtime + "/out";
        runWhisperMess(model, wav, out);
        copyToClipboardHack("transcribed text here lol");
        sendNotif("copied!");
        lastPid = -1;
    }
}

void uninstallEverything() {
    cout << "uninstalling everything hope it works\n";
    try {
        filesystem::remove_all("/home/user/.local/share/asryx");
        filesystem::remove("/home/user/.asryx.conf");
    } catch(...) {}
    cout << "done i think\n";
}

int main(int argc, char** argv) {
    cout << "ASRYX PROTOTYPE v0.0.1 - chaotic edition\n";
    
    vector<string> args;
    for(int i=1; i<argc; i++) {
        args.push_back(argv[i]);
    }
    
    int x = 69;
    string y = "unused";
    
    return doTheThing(args);
}
#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <string>
#include <map>

// use c++11 option to compile this file:
// g++ -std=c++11 analyze.cpp -o analyze

using namespace std;

class Result {
public:
    string answer;
    string cputime;
    string walltime;
    string mem;

    string str() {
        return answer + ", " + cputime + ", " + walltime + ", " + mem;
    }
};

class Answer {
public:
    string aprove;
    string ceagle;
    string ctrl;
    string kittel;

    string str() {
        return aprove + ", " + ceagle + ", " + ctrl + ", " + kittel;
    }

    string formattedStr() {
        return aprove + "\t" + ceagle + "\t" + ctrl + "\t" + kittel;
    }
};

class Stat {
public:
    int termination = 0;
    int timeout = 0;
    int failed = 0;
};

class Comparison {
public:
    int ceagle_win = 0;
    int ceagle_lose = 0;
    int both_win = 0;
    int both_lose = 0;
};

bool isInstFail(string casename) {
    string fn = casename.substr(casename.find_last_of('/') + 1);
    //cout << fn << endl;
    string buffer;
    bool result = false;

    ifstream logFile("ceagle.logfiles/default." + fn + ".log");
    if (logFile.is_open()) {
        while(getline(logFile, buffer)) {
            if (buffer.find("Unknown!!!") != string::npos) {
                result = true;
                break;
            }
        }
        logFile.close();
    }

    return result;
}

int main() {
    string line;
    string segment, casename;

    //////////////////////////////////////////////////////////
    // Ctrl results
    //////////////////////////////////////////////////////////
    std::map<string, Result> ctrl;
    ifstream ctrlFile("ctrl.csv");

    if (ctrlFile.is_open()) {
        for (int i = 0; i < 3; i++) {
            getline(ctrlFile, line);
        }

        while (getline(ctrlFile,line)) {
            istringstream is(line);

            getline(is, casename, '\t');

            Result result;

            getline(is, segment, '\t');
            result.answer = segment;

            getline(is, segment, '\t');
            result.cputime = segment;

            getline(is, segment, '\t');
            result.walltime = segment;

            getline(is, segment, '\t');
            result.mem = segment;

            ctrl[casename] = result;
        }

        ctrlFile.close();
    }

    //cout << "Ctrl: " << ctrl.size() << endl;
    map<string, Result> ctrlTermination, ctrlNonTermination, ctrlUnsafe;
    for (auto i = ctrl.begin(); i != ctrl.end(); i++) {
        if ((*i).first.find("true") != string::npos) {
            ctrlTermination.insert(*i);
        } else if ((*i).first.find("false") != string::npos) {
            ctrlNonTermination.insert(*i);
        } else if ((*i).first.find("unsafe") != string::npos) {
            ctrlUnsafe.insert(*i);
        }
        // else cout << (*i).first << endl;
        // note that there is an unknown test in the benchmark:
        // C/Ultimate/Collatz_unknown-termination.c
    }
    /*
    cout << "Termination: " << ctrlTermination.size() << endl;
    cout << "NonTermination: " << ctrlNonTermination.size() << endl;
    cout << "Unsafe: " << ctrlUnsafe.size() << endl << endl;
    */

    //////////////////////////////////////////////////////////
    // AProVE results
    //////////////////////////////////////////////////////////

    std::map<string, Result> aprove;
    ifstream aproveFile("aprove.csv");

    if (aproveFile.is_open()) {
        for (int i = 0; i < 3; i++) {
            getline(aproveFile, line);
        }

        while (getline(aproveFile,line)) {
            istringstream is(line);

            getline(is, casename, '\t');

            Result result;

            getline(is, segment, '\t');
            result.answer = segment;

            getline(is, segment, '\t');
            result.cputime = segment;

            getline(is, segment, '\t');
            result.walltime = segment;

            getline(is, segment, '\t');
            result.mem = segment;

            aprove[casename] = result;
        }

        aproveFile.close();
    }

    //cout << "AProVE: " << aprove.size() << endl;
    map<string, Result> aproveTermination, aproveNonTermination, aproveUnsafe;
    for (auto i = aprove.begin(); i != aprove.end(); i++) {
        if ((*i).first.find("true") != string::npos) {
            aproveTermination.insert(*i);
        } else if ((*i).first.find("false") != string::npos) {
            aproveNonTermination.insert(*i);
        } else if ((*i).first.find("unsafe") != string::npos) {
            aproveUnsafe.insert(*i);
        }
        // else cout << (*i).first << endl;
        // note that there is an unknown test in the benchmark:
        // C/Ultimate/Collatz_unknown-termination.c
    }
    /*
    cout << "Termination: " << aproveTermination.size() << endl;
    cout << "NonTermination: " << aproveNonTermination.size() << endl;
    cout << "Unsafe: " << aproveUnsafe.size() << endl << endl;
    */


    //////////////////////////////////////////////////////////
    // Ceagle results
    //////////////////////////////////////////////////////////

    std::map<string, Result> ceagle;
    ifstream ceagleFile("ceagle.csv");

    if (ceagleFile.is_open()) {
        for (int i = 0; i < 3; i++) {
            getline(ceagleFile, line);
        }

        while (getline(ceagleFile,line)) {
            istringstream is(line);

            getline(is, casename, '\t');

            Result result;

            getline(is, segment, '\t');
            result.answer = segment;

            getline(is, segment, '\t');
            result.cputime = segment;

            getline(is, segment, '\t');
            result.walltime = segment;

            getline(is, segment, '\t');
            result.mem = segment;

            ceagle[casename] = result;
        }

        ceagleFile.close();
    }

    // cout << "Ceagle: " << ceagle.size() << endl;
    map<string, Result> ceagleTermination, ceagleNonTermination, ceagleUnsafe;
    for (auto i = ceagle.begin(); i != ceagle.end(); i++) {
        if ((*i).first.find("true") != string::npos) {
            ceagleTermination.insert(*i);
        } else if ((*i).first.find("false") != string::npos) {
            ceagleNonTermination.insert(*i);
        } else if ((*i).first.find("unsafe") != string::npos) {
            ceagleUnsafe.insert(*i);
        }
        // else cout << (*i).first << endl;
        // note that there is an unknown test in the benchmark:
        // C/Ultimate/Collatz_unknown-termination.c
    }
    /*
    cout << "Termination: " << ceagleTermination.size() << endl;
    cout << "NonTermination: " << ceagleNonTermination.size() << endl;
    cout << "Unsafe: " << ceagleUnsafe.size() << endl << endl;
     */



    //////////////////////////////////////////////////////////
    // KITTeL results
    //////////////////////////////////////////////////////////

    std::map<string, Result> kittel;
    ifstream kittelFile("kittel.csv");

    if (kittelFile.is_open()) {
        for (int i = 0; i < 3; i++) {
            getline(kittelFile, line);
        }

        while (getline(kittelFile,line)) {
            istringstream is(line);

            getline(is, casename, '\t');

            Result result;

            getline(is, segment, '\t');
            result.answer = segment;

            getline(is, segment, '\t');
            result.cputime = segment;

            getline(is, segment, '\t');
            result.walltime = segment;

            getline(is, segment, '\t');
            result.mem = segment;

            kittel[casename] = result;
        }

        kittelFile.close();
    }

    // cout << "KITTeL: " << kittel.size() << endl;
    map<string, Result> kittelTermination, kittelNonTermination, kittelUnsafe;
    for (auto i = kittel.begin(); i != kittel.end(); i++) {
        if ((*i).first.find("true") != string::npos) {
            kittelTermination.insert(*i);
        } else if ((*i).first.find("false") != string::npos) {
            kittelNonTermination.insert(*i);
        } else if ((*i).first.find("unsafe") != string::npos) {
            kittelUnsafe.insert(*i);
        }
        // else cout << (*i).first << endl;
        // note that there is an unknown test in the benchmark:
        // C/Ultimate/Collatz_unknown-termination.c
    }
    /*
    cout << "Termination: " << kittelTermination.size() << endl;
    cout << "NonTermination: " << kittelNonTermination.size() << endl;
    cout << "Unsafe: " << kittelUnsafe.size() << endl << endl;
    */


    //////////////////////////////////////////////////////////
    // extract only the answers from these tools
    //////////////////////////////////////////////////////////

    map<string, Answer> C_track, C_Int_track;
    for (auto i = aproveTermination.begin(); i != aproveTermination.end(); i++) {
        casename = (*i).first;
        Answer answer;
        answer.aprove = aproveTermination[casename].answer;
        answer.ceagle = ceagleTermination[casename].answer;
        answer.ctrl = ctrlTermination[casename].answer;
        answer.kittel = kittelTermination[casename].answer;

        // the C_Integer dir
        if (casename.find("C_Integer") != string::npos) {
            C_Int_track[casename] = answer;
        } else { // the C dir
            C_track[casename] = answer;
        }
    }

    cout << "C_Integer: " << C_Int_track.size() << endl;
    cout << "C: " << C_track.size() << endl << endl;


    //////////////////////////////////////////////////////////
    // check which files fail due to instruction
    //////////////////////////////////////////////////////////

    for (auto i = C_Int_track.begin(); i != C_Int_track.end(); i++) {
        casename = (*i).first;
        if ((*i).second.ceagle != "true" && (*i).second.ceagle != "TIMEOUT") {
            if (isInstFail(casename)) {
                if ((*i).second.aprove == "true" &&  (*i).second.kittel == "true") {
                    (*i).second.ceagle = "true";
                }
            }
        }
    }
    for (auto i = C_track.begin(); i != C_track.end(); i++) {
        casename = (*i).first;
        if ((*i).second.ceagle != "true" && (*i).second.ceagle != "TIMEOUT") {
            if (isInstFail(casename)) {
                if ((*i).second.aprove == "true" &&  (*i).second.kittel == "true") {
                    (*i).second.ceagle = "true";
                }
            }
        }
    }



    //////////////////////////////////////////////////////////
    // evaluation for each tool
    //////////////////////////////////////////////////////////

    Stat statAprove, statCeagle, statCtrl, statKittel;
    Stat statAprove_int, statCeagle_int, statCtrl_int, statKittel_int;

    // evaluation for C_integer
    for (auto i = C_Int_track.begin(); i != C_Int_track.end(); i++) {
        // aprove
        if ((*i).second.aprove == "true") {
            statAprove_int.termination++;
        } else if ((*i).second.aprove == "TIMEOUT") {
            statAprove_int.timeout++;
        } else {
            statAprove_int.failed++;
        }

        // ceagle
        if ((*i).second.ceagle == "true") {
            statCeagle_int.termination++;
        } else if ((*i).second.ceagle == "TIMEOUT") {
            statCeagle_int.timeout++;
        } else {
            statCeagle_int.failed++;
        }

        // ctrl
        if ((*i).second.ctrl == "true") {
            statCtrl_int.termination++;
        } else if ((*i).second.ctrl == "TIMEOUT") {
            statCtrl_int.timeout++;
        } else {
            statCtrl_int.failed++;
        }

        // kittel
        if ((*i).second.kittel == "true") {
            statKittel_int.termination++;
        } else if ((*i).second.kittel == "TIMEOUT") {
            statKittel_int.timeout++;
        } else {
            statKittel_int.failed++;
        }
    }
    cout << "C_Integer" << endl;
    cout << "AProVE: termination - " << statAprove_int.termination
            << ", timeout - " << statAprove_int.timeout
            << ", unknown - " << statAprove_int.failed << endl;
    cout << "Ceagle: termination - " << statCeagle_int.termination
            << ", timeout - " << statCeagle_int.timeout
            << ", unknown - " << statCeagle_int.failed << endl;
    cout << "Ctrl: termination - " << statCtrl_int.termination
            << ", timeout - " << statCtrl_int.timeout
            << ", unknown - " << statCtrl_int.failed << endl;
    cout << "KITTeL: termination - " << statKittel_int.termination
            << ", timeout - " << statKittel_int.timeout
            << ", unknown - " << statKittel_int.failed << endl;

    // evaluation for C
    for (auto i = C_track.begin(); i != C_track.end(); i++) {
        // aprove
        if ((*i).second.aprove == "true") {
            statAprove.termination++;
        } else if ((*i).second.aprove == "TIMEOUT") {
            statAprove.timeout++;
        } else {
            statAprove.failed++;
        }

        // ceagle
        if ((*i).second.ceagle == "true") {
            statCeagle.termination++;
        } else if ((*i).second.ceagle == "TIMEOUT") {
            statCeagle.timeout++;
        } else {
            statCeagle.failed++;
        }

        // ctrl
        if ((*i).second.ctrl == "true") {
            statCtrl.termination++;
        } else if ((*i).second.ctrl == "TIMEOUT") {
            statCtrl.timeout++;
        } else {
            statCtrl.failed++;
        }

        // kittel
        if ((*i).second.kittel == "true") {
            statKittel.termination++;
        } else if ((*i).second.kittel == "TIMEOUT") {
            statKittel.timeout++;
        } else {
            statKittel.failed++;
        }
    }
    cout << "C" << endl;
    cout << "AProVE: termination - " << statAprove.termination
            << ", timeout - " << statAprove.timeout
            << ", unknown - " << statAprove.failed << endl;
    cout << "Ceagle: termination - " << statCeagle.termination
            << ", timeout - " << statCeagle.timeout
            << ", unknown - " << statCeagle.failed << endl;
    cout << "Ctrl: termination - " << statCtrl.termination
            << ", timeout - " << statCtrl.timeout
            << ", unknown - " << statCtrl.failed << endl;
    cout << "KITTeL: termination - " << statKittel.termination
            << ", timeout - " << statKittel.timeout
            << ", unknown - " << statKittel.failed << endl;


    //////////////////////////////////////////////////////////
    // Comparison between Ceagle and other tools
    //////////////////////////////////////////////////////////

    // C_Integer track
    Comparison withAP_int, withCL_int, withKT_int;
    for (auto i = C_Int_track.begin(); i != C_Int_track.end(); i++) {
        if ((*i).second.ceagle == "true") {
            // compare to aprove
            if ((*i).second.aprove == "true") {
                withAP_int.both_win++;
            } else {
                withAP_int.ceagle_win++;
            }
            // compare to ctrl
            if ((*i).second.ctrl == "true") {
                withCL_int.both_win++;
            } else {
                withCL_int.ceagle_win++;
            }
            // compare to kittel
            if ((*i).second.kittel == "true") {
                withKT_int.both_win++;
            } else {
                withKT_int.ceagle_win++;
            }
        } else { // ceagle timeout or fails
            // compare to aprove
            if ((*i).second.aprove == "true") {
                withAP_int.ceagle_lose++;
            } else {
                withAP_int.both_lose++;
            }
            // compare to ctrl
            if ((*i).second.ctrl == "true") {
                withCL_int.ceagle_lose++;
            } else {
                withCL_int.both_lose++;
            }
            // compare to kittel
            if ((*i).second.kittel == "true") {
                withKT_int.ceagle_lose++;
            } else {
                withKT_int.both_lose++;
            }
        }
    }

    // C track
    Comparison withAP, withCL, withKT;
    for (auto i = C_track.begin(); i != C_track.end(); i++) {
        if ((*i).second.ceagle == "true") {
            // compare to aprove
            if ((*i).second.aprove == "true") {
                withAP.both_win++;
            } else {
                withAP.ceagle_win++;
            }
            // compare to ctrl
            if ((*i).second.ctrl == "true") {
                withCL.both_win++;
            } else {
                withCL.ceagle_win++;
            }
            // compare to kittel
            if ((*i).second.kittel == "true") {
                withKT.both_win++;
            } else {
                withKT.ceagle_win++;
            }
        } else { // ceagle timeout or fails
            // compare to aprove
            if ((*i).second.aprove == "true") {
                withAP.ceagle_lose++;
            } else {
                withAP.both_lose++;
            }
            // compare to ctrl
            if ((*i).second.ctrl == "true") {
                withCL.ceagle_lose++;
            } else {
                withCL.both_lose++;
            }
            // compare to kittel
            if ((*i).second.kittel == "true") {
                withKT.ceagle_lose++;
            } else {
                withKT.both_lose++;
            }
        }
    }

    cout << endl;

    cout << "For C_Integer programs:" << endl;
    cout << "With AProVE: Ceagle win - " << withAP_int.ceagle_win
            << ", AProVE win - " << withAP_int.ceagle_lose
            << ", both suc - " << withAP_int.both_win
            << ", both fail - " << withAP_int.both_lose << endl;
    cout << "With Ctrl: Ceagle win - " << withCL_int.ceagle_win
            << ", Ctrl win - " << withCL_int.ceagle_lose
            << ", both suc - " << withCL_int.both_win
            << ", both fail - " << withCL_int.both_lose << endl;
    cout << "With KITTeL: Ceagle win - " << withKT_int.ceagle_win
            << ", KITTeL win - " << withKT_int.ceagle_lose
            << ", both suc - " << withKT_int.both_win
            << ", both fail - " << withKT_int.both_lose << endl;

    cout << "For C programs:" << endl;
    cout << "With AProVE: Ceagle win - " << withAP.ceagle_win
            << ", AProVE win - " << withAP.ceagle_lose
            << ", both suc - " << withAP.both_win
            << ", both fail - " << withAP.both_lose << endl;
    cout << "With Ctrl: Ceagle win - " << withCL.ceagle_win
            << ", Ctrl win - " << withCL.ceagle_lose
            << ", both suc - " << withCL.both_win
            << ", both fail - " << withCL.both_lose << endl;
    cout << "With KITTeL: Ceagle win - " << withKT.ceagle_win
            << ", KITTeL win - " << withKT.ceagle_lose
            << ", both suc - " << withKT.both_win
            << ", both fail - " << withKT.both_lose << endl;





    //////////////////////////////////////////////////////////
    // Count num of instruction fail in Ceagle
    //////////////////////////////////////////////////////////
    int k = 0;
    int k_a = 0;
    int k_k = 0;
    int k_ak = 0;
    for (auto i = C_Int_track.begin(); i != C_Int_track.end(); i++) {
        casename = (*i).first;
        if ((*i).second.ceagle != "true" && (*i).second.ceagle != "TIMEOUT") {
            if (isInstFail(casename)) {
                k++;
                if ((*i).second.aprove == "true") {
                    k_a++;
                }
                if ((*i).second.kittel == "true") {
                    k_k++;
                }
                if ((*i).second.aprove == "true" &&  (*i).second.kittel == "true") {
                    k_ak++;
                }
            }
        }
    }
    cout << "C Integer num: " << k << endl;
    cout << "aprove can do " << k_a << ", kittel can do " << k_k
            << ", both can do " << k_ak << endl;

    k = 0;
    k_a = 0;
    k_k = 0;
    k_ak = 0;
    for (auto i = C_track.begin(); i != C_track.end(); i++) {
        casename = (*i).first;
        if ((*i).second.ceagle != "true" && (*i).second.ceagle != "TIMEOUT") {
            if (isInstFail(casename)) {
                k++;
                if ((*i).second.aprove == "true") {
                    k_a++;
                }
                if ((*i).second.kittel == "true") {
                    k_k++;
                }
                if ((*i).second.aprove == "true" &&  (*i).second.kittel == "true") {
                    k_ak++;
                }
            }
        }
    }
    cout << "C num: " << k << endl;
    cout << "aprove can do " << k_a << ", kittel can do " << k_k
            << ", both can do " << k_ak << endl;


    // count the num of files which no one can do
    int no_one_int = 0;
    for (auto i = C_Int_track.begin(); i != C_Int_track.end(); i++) {
        if ((*i).second.aprove != "true" &&  (*i).second.kittel != "true"
                && (*i).second.ceagle != "true" && (*i).second.ctrl != "true") {
            no_one_int++;
        }
    }
    cout << endl;
    int no_one = 0;
    for (auto i = C_track.begin(); i != C_track.end(); i++) {
        if ((*i).second.aprove != "true" &&  (*i).second.kittel != "true"
                && (*i).second.ceagle != "true" && (*i).second.ctrl != "true") {
            no_one++;
        }

    }
    cout << "C_Integer: no one can do - " << no_one_int << endl;
    cout << "C: no one can do - " << no_one << endl;

    return 0;
}

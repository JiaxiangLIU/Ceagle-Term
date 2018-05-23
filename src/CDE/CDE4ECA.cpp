#include <fstream>
#include <iostream>
#include <regex>
#include <stdlib.h>
#include <ctype.h>
#include <ctime>
#include <chrono>
#include <vector>
#include <Witness/ViolationWitness.h>
#include <Witness/CorrectnessWitness.h>
#include "CDE.h"

using namespace std;

pair<int, int> wrap(const char* inputFile, const char* outputFile) {
    ifstream fin(inputFile);
    ofstream fout(outputFile);

    if (!fin.is_open() || !fout.is_open()) {
        throw("Cannot open input/output file.");
    }

    fout << "#include<iostream>" << endl;
	fout << "#include<map>" << endl;
	fout << "#include<queue>" << endl;
	fout << "#include<vector>" << endl;
	fout << "#include<stdlib.h>" << endl;
	fout << "#include<signal.h>" << endl;
    fout << "class CDE{" << endl;
    fout << "public:" << endl;
    fout << "void __VERIFIER_error() {throw \"error\";}" << endl;
    fout << "void exit(int code) {throw code;}" << endl;

	regex fdec(R"(^\W*(extern\W)?(int|void)\W+\w+\W*\(.*\)\W*;\s*$)");
	regex vdef(R"(^\W*int\s+\w+(\s*=\s*-?\s*\d+)\s*;\s*$)");
	regex fdef(R"(^.*\Wcalculate_output\W[^;]*$)");
	regex mf(R"(^.*\smain\W*\(.*\)\W*$)");
	vector<string> vars;

	string line;
    while (getline(fin, line)) {
        if (regex_match(line, fdec)) {
			// skip
        } else if (regex_match(line, vdef)) {
			int i = line.find("int") + 3;
			while (!isalpha(line[i]) && line[i] != '_') {
				++ i;
			}
			int j = i + 1;
			while (isalpha(line[j]) || isdigit(line[j]) || line[j] == '_') {
				++ j;
			}
			string var = line.substr(i, j - i);
			// there are only some vars needed
			if (var.length() > 0) {
				fout << line << endl;
				vars.push_back(var);
			} else {
				fout << "//" << line << endl;
			}
		} else if (regex_match(line, fdef)) {
            break;
        } else {
            fout << line << endl;
        }
    }

	fout << "std::vector<int> seq;\n" << endl;

	fout << line << endl;
	while (getline(fin, line)) {
		if (regex_match(line, mf)) {
			fout << "};" << endl;
			break;
		} else {
			fout << line << endl;
		}
	}

    fin.seekg(fin.beg);
    int cnt1 = 0;
    int cnt2 = 0;
    int cnt = 0;
    while (1) {
        ++ cnt;
        getline(fin, line);
        if (line.find("__VERIFIER_error") != string::npos) {
            cnt1 = cnt;
        }
        if (line.find("input") != string::npos && line.find("__VERIFIER_nondet_int") != string::npos) {
            cnt2 = cnt;
            break;
        }
    }

	fout << "struct cdeptr_less {\n"
			"    bool operator()(CDE* lhs, CDE* rhs) {\n";
	for (int i = 0; i < vars.size(); ++ i) {
		fout << "        if (lhs->" << vars[i] << " < rhs->" << vars[i] << ") {" << endl;
		fout << "            return 1;" << endl;
		fout << "        } else if (lhs->" << vars[i] << " > rhs->" << vars[i] << ") {" << endl;
		fout << "            return 0;" << endl;
		fout << "        }" << endl;
	}
	fout << "        return 0;" << endl;
	fout << "    }" << endl;
	fout << "};\n" << endl;

	fout << "int flag = 0;\n" << endl;

	fout << "void handleTermination(int arg) {\n"
	        "    flag = 1;\n"
	        "}\n" << endl;

    fout << "int main()\n"
            "{\n"
            "    // default output\n"
            "    int output = -1;\n"
            "\n"
			"    void (*timeoutHandler)(int);\n"
			"    timeoutHandler = signal(SIGTERM, handleTermination);\n"
            "\n"
            "    std::map<CDE*, int, cdeptr_less> exploration;\n"
            "    std::queue<CDE*> worklist;\n"
            "\n"
            "    CDE* instance = new CDE();\n"
			"    exploration[instance] = 1;\n"
			"    worklist.push(instance);\n"
            "\n"
            "    int ret = 1;\n"
            "\n";
    fout << "    while(!worklist.empty()) {\n"
	        "        if (flag) {\n"
			"            ret = -1;\n"
			"            break;\n"
			"        }\n"
	        "        if (exploration.size() > 32000) {\n"
			"            ret = -1;\n"
			"            break;\n"
			"        }\n"
            "        instance = worklist.front();\n"
			"        worklist.pop();\n"
			"        int i = 1;\n"
            "        try {\n"
            "            for (; i < 7; ++ i) {\n";

    while (getline(fin, line)) {
        if (line.find("if ((i != ") != string::npos) {
            if (line.find("i != 1") == string::npos) {
                fout << "                if (i[i] == 1) continue;" << endl;
            }
            if (line.find("i != 2") == string::npos) {
                fout << "                if (i[i] == 2) continue;" << endl;
            }
            if (line.find("i != 3") == string::npos) {
                fout << "                if (i[i] == 3) continue;" << endl;
            }
            if (line.find("i != 4") == string::npos) {
                fout << "                if (i[i] == 4) continue;" << endl;
            }
            if (line.find("i != 5") == string::npos) {
                fout << "                if (i[i] == 5) continue;" << endl;
            }
            if (line.find("i != 6") == string::npos) {
                fout << "                if (i[i] == 6) continue;" << endl;
            }
        }
    }

	fout << "                CDE* next_instance = new CDE(*instance);\n"
			"                next_instance->calculate_output(i);\n"
			"                std::pair<std::map<CDE*, int>::iterator, std::map<CDE*, int>::iterator> range = exploration.equal_range(next_instance);\n"
			"                if (range.first != range.second) {\n"
			"                    delete next_instance;\n"
			"                    next_instance = 0;\n"
			"                } else {\n"
			"                    next_instance->seq.push_back(i);\n"
			"                    worklist.push(next_instance);\n"
			"                    exploration.emplace_hint(range.first, next_instance, 1);\n"
	        "                }\n"
            "            }\n"
            "        } catch (int code) {\n"
            "\n"
            "        } catch (const char* str) {\n"
            "            std::cout << \"[CE:\";\n"
            "            for (int j = 0; j < instance->seq.size(); ++ j) {\n"
            "                std::cout << instance->seq[j] << \" \";\n"
            "            }\n"
            "            std::cout << i << \"]\";\n";
    fout << "            std::cout << \"[WITNESS\";\n"
            "            CDE* tmp_instance = new CDE();\n"
            "            for (int j = 0; j < instance->seq.size(); ++ j) {\n"
            "                std::cout << \"input == (\" << instance->seq[j] << \")\";\n";
    for (string s : vars) {
        fout << "                std::cout << \"; " << s << " == (\" << tmp_instance->" << s << " << \")\";\n";
    }
    fout << "                std::cout << \"WITNESS\";\n"
            "                tmp_instance->calculate_output(instance->seq[j]);\n"
            "            }\n"
            "            std::cout << \"input == (\" << i << \")WITNESS]\";\n";
    fout << "            ret = 0;\n"
			"            break;"
            "        }\n"
            "    }\n"
			"    if (ret == 0) {\n"
			"        std::cout << \"[FALSE]\";\n"
			"    } else if (ret == 1) {\n"
			"        std::cout << \"[TRUE]\";\n"
			"    } else {\n"
			"        std::cout << \"[UNKNOWN]\";\n"
			"    }\n"
            "    std::cout << \"[STATES:\" << exploration.size() << \"]\";\n"
            "    return ret;\n"
            "}" << endl;

    fin.close();
    fout.close();

    return pair<int, int>(cnt1, cnt2);
}

namespace ceagle {

namespace cde {

int runECA(const char *sourceFile, int timeLimit) {
    int ret = 0;

    string outputFile(sourceFile);
    outputFile = outputFile + "pp";
    pair<int, int> lnNondet = wrap(sourceFile, outputFile.c_str());

    FILE *f = fopen(outputFile.c_str(), "r");
    if (f == NULL) {
        cout << "CDE Generation Failed" << endl;
        return 0;
    } else {
        fclose(f);
    }

    string binaryFile = outputFile.substr(0, outputFile.length() - 3) + "out";
    string command = "g++ -std=c++11 " + outputFile + " -o " + binaryFile;
    system(command.c_str());

    f = fopen(binaryFile.c_str(), "r");
    if (f == NULL) {
        cout << "CDE Compilation Failed" << endl;
        command = "rm " + outputFile;
        system(command.c_str());
        return 0;
    } else {
        fclose(f);
    }

    FILE *pipe;
    command = "timeout " + to_string(timeLimit) + " ./" + binaryFile;
    char buffer[65536];

    if (pipe = popen(command.c_str(), "r")) {
        fgets(buffer, sizeof(buffer), pipe);
        pclose(pipe);

        string result(buffer);
        if (result.find("TRUE") != string::npos) {
            ret = 1;
        } else if (result.find("FALSE") != string::npos) {
            ret = -1;
        } else if (result.find("UNKNOWN") != string::npos) {
            ret = 1;
        } else {
            ret = 0;
        }
    } else {
        cout << "CDE Execution Failed" << endl;
        command = "rm " + outputFile + " " + binaryFile;
        system(command.c_str());
        return 0;
    }

    command = "rm " + outputFile + " " + binaryFile;
    system(command.c_str());

    if (ret == -1) {
        ViolationWitness vw(sourceFile);
        vw.initEmpty();
        string witness = vw.getWitness();
        unsigned long pos1 = witness.find("<node");
        unsigned long pos2 = witness.find("</node>");
        string witness1 = witness.substr(0, pos1);
        string witness2 = witness.substr(pos2 + 7);

        witness1 = witness1 + "<node id=\"N0\">\n<data key=\"entry\">true</data>\n</node>\n";

        witness = string(buffer);
        pos1 = witness.find("WITNESS", 0);
        pos2 = witness.find("WITNESS", pos1 + 7);
        int cnt = 1;
        while (pos2 != string::npos) {
            string assumption = witness.substr(pos1 + 7, pos2 - pos1 - 7);
            witness1 = witness1 + "<node id=\"N" + to_string(cnt) + "\"/>\n";
            witness1 = witness1 + "<edge source=\"N" + to_string(cnt - 1) + "\" target=\"N" + to_string(cnt) + "\">\n" + "<data key=\"sourcecode\">input = __VERIFIER_nondet_int();</data>\n" + "<data key=\"startline\">" + to_string(lnNondet.second) + "</data>\n" + "<data key=\"assumption\">" + assumption + "</data>\n" + "<data key=\"assumption.scope\">main</data>\n" + "</edge>\n";

            pos1 = pos2;
            pos2 = witness.find("WITNESS", pos1 + 6);
            ++ cnt;
        }

        witness1 = witness1 + "<node id=\"NE\">\n<data key=\"violation\">true</data>\n</node>\n" + "<edge source=\"N" + to_string(cnt - 1) + "\" target=\"NE\">\n" + "<data key=\"sourcecode\">error_17: __VERIFIER_error();</data>\n" + "<data key=\"startline\">" + to_string(lnNondet.first) + "</data>\n" + "</edge>";
        witness = witness1 + witness2;

        string wfName = "witness.graphml";
        ofstream fout(wfName.c_str());
        fout << witness << endl;
        fout.close();
    } else if (ret == 1) {
        CorrectnessWitness cw(sourceFile);
        cw.initEmpty();
        ofstream fout("witness.graphml");
        fout << cw.getWitness() << endl;
        fout.close();
    }

    return ret;
}

}

}

/*
int main(int argc, char** argv) {
    ofstream fout("result.txt");

	int timeLimit = atoi(argv[1]);
    for (int i = 2; i < argc; ++ i) {
        string outputFile(argv[i]);

        if (outputFile.find("true") != string::npos) {
            // continue;
        }

        auto t1 = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();

        outputFile = outputFile + "pp";
        wrap(argv[i], outputFile.c_str());

        auto t2 = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();

        string binaryFile = outputFile.substr(0, outputFile.length() - 3) + "out";
        string command = "g++ -std=c++11 " + outputFile + " -o " + binaryFile;
        system(command.c_str());

		auto t3 = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();

        FILE* f = fopen(binaryFile.c_str(), "r");
        if (f == NULL) {
            fout << argv[i] << "\t" << "CDE Fail" << endl;
        } else {
            fclose(f);

            fout << argv[i] << "\t";

            FILE* pipe;
            command = "timeout " + to_string(timeLimit) + " ./" + binaryFile + " -s USR1";
            char buffer[256];

            if (pipe = popen(command.c_str(), "r")) {
                fgets(buffer, sizeof(buffer), pipe);

				auto t4 = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();

				float d1 = (t2 - t1) * 1.0 / 1000;
                float d2 = (t3 - t2) * 1.0 / 1000;
				float d3 = (t4 - t3) * 1.0 / 1000;

				pclose(pipe);
				fout << d1 << "s\t" << d2 << "s\t" << d3 << "s\t" << buffer;
				cout << argv[i] << buffer;
            } else {
                fout << "Call popen Fail" << endl;
            }
        }
    }
    fout.close();
}
*/
/*
int main(int argc, char** argv) {
	int timeLimit = atoi(argv[1]);
    for (int i = 2; i < argc; ++ i) {
		int result = CDERun(argv[i], timeLimit);
		cout << result << endl;
	}
}
*/

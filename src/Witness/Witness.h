#ifndef CEAGLE_WITNESS_H
#define CEAGLE_WITNESS_H

#include "../Util/StringUtil.h"
#include <hashlibpp.h>

#include <string>
#include <vector>
#include <map>

using std::string;
using std::vector;
using std::map;
using std::to_string;
using ceagle::util::getGraphmlStr;

namespace ceagle {

    namespace wg {

        class Edge;

        class Node {
        public:
            vector<Edge *> ins;
            vector<Edge *> outs;
            string name;
            bool isEntry;
            bool isViolation;
            string invariant;
            string invariantScope;

            Node(string name, bool isEntry, bool isViolation, string invariant, string invariantScope) {
                this->ins = vector<Edge *>();
                this->outs = vector<Edge *>();
                this->name = name;
                this->isEntry = isEntry;
                this->isViolation = isViolation;
                this->invariant = invariant;
                this->invariantScope = invariantScope;
            }

            Node(bool isEntry, bool isViolation) : Node("", isEntry, isViolation, "", "") {}

            Node(string invariant, string invariantScope) : Node("", 0, 0, invariant, invariantScope) {}

            Node() : Node("", 0, 0, "", "") {}
        };

        class Edge {
        public:
            Node *start;
            Node *end;
            string srcCode;
            int line;
            int column;
            int control;
            string assumption; // concrete values in the statement.e.g a=2
            string assumptionScope; // function level scope

            Edge(Node *start, Node *end, string srcCode, int line, int column, int control, string assumption,
                 string assumptionScope) {
                this->start = start;
                start->outs.push_back(this);
                this->end = end;
                end->ins.push_back(this);
                this->srcCode = srcCode;
                this->line = line;
                this->column = column;
                this->control = control;
                this->assumption = assumption;
                this->assumptionScope = assumptionScope;
            }

            Edge(Node *start, Node *end, string srcCode, int line, int column, int control) : Edge(start, end, srcCode,
                                                                                                   line, column,
                                                                                                   control, "", "") {}

            Edge(Node *start, Node *end, string srcCode, int line, int column, string assumption,
                 string assumptionScope) : Edge(start, end, srcCode, line, column, 0, assumption, assumptionScope) {}

            Edge(Node *start, Node *end, string srcCode, int line, int column) : Edge(start, end, srcCode, line, column,
                                                                                      0, "", "") {}
        };

    }

    using namespace wg;

    enum WitnessType {
        VIOLATION, CORRECTNESS
    };

    class Witness {
    protected:
        map<string, string> taskInfo;
        wg::Node *entry;
        vector<wg::Node *> nodes;
        vector<wg::Edge *> edges;

        Witness(string srcFile, const map<string, string> &taskInfo) {
            FILE *f = fopen(srcFile.c_str(), "r");
            if (!f) {
                throwStr("[Witness.h constructor] Require correct file name with absolute path");
            }
            fclose(f);
            this->taskInfo = map<string, string>();
            this->taskInfo["sourcecodelang"] = "C";
            this->taskInfo["producer"] = "Ceagle";
            this->taskInfo["specification"] = "CHECK( init(main()), LTL(G ! call(__VERIFIER_error())) )";
            this->taskInfo["programfile"] = srcFile;
            this->taskInfo["memorymodel"] = "precise";
            this->taskInfo["architecture"] = "32bit";
            for (map<string, string>::const_iterator i = taskInfo.begin(); i != taskInfo.end(); ++i) {
                this->taskInfo[i->first] = i->second;
            }
            this->nodes = vector<wg::Node *>();
            this->edges = vector<wg::Edge *>();
        }

        void throwStr(string str) { throw (str); }

    public:
        void init(wg::Node *entry) { this->entry = entry; }

        void add(wg::Node *node) { this->nodes.push_back(node); }

        void add(wg::Edge *edge) { this->edges.push_back(edge); }

        string getWitness() {
            string fileHash = "";
            sha1wrapper *w = new sha1wrapper();
            try {
                fileHash = w->getHashFromFile(taskInfo["programfile"]);
            } catch (hlException &e) {
                throwStr("[Witness.h constructor] Fail to calculate program file hash");
            }
            delete w;
            w = NULL;

            string ret = "";
            ret = ret + "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n";
            ret = ret +
                  "<graphml xmlns=\"http://graphml.graphdrawing.org/xmlns\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">\n";
            ret = ret + "<key attr.name=\"witness-type\" attr.type=\"string\" for=\"graph\" id=\"witness-type\"/>\n";
            ret = ret +
                  "<key attr.name=\"sourcecodelang\" attr.type=\"string\" for=\"graph\" id=\"sourcecodelang\"/>\n";
            ret = ret + "<key attr.name=\"producer\" attr.type=\"string\" for=\"graph\" id=\"producer\"/>\n";
            ret = ret + "<key attr.name=\"specification\" attr.type=\"string\" for=\"graph\" id=\"specification\"/>\n";
            ret = ret + "<key attr.name=\"programfile\" attr.type=\"string\" for=\"graph\" id=\"programfile\"/>\n";
            ret = ret + "<key attr.name=\"programhash\" attr.type=\"string\" for=\"graph\" id=\"programhash\"/>\n";
            ret = ret + "<key attr.name=\"memorymodel\" attr.type=\"string\" for=\"graph\" id=\"memorymodel\"/>\n";
            ret = ret + "<key attr.name=\"architecture\" attr.type=\"string\" for=\"graph\" id=\"architecture\"/>\n";
            ret = ret +
                  "<key attr.name=\"entry\" attr.type=\"boolean\" for=\"node\" id=\"entry\">\n<default>false</default>\n</key>\n";
            ret = ret +
                  "<key attr.name=\"violation\" attr.type=\"boolean\" for=\"node\" id=\"violation\">\n<default>false</default>\n</key>\n";
            ret = ret + "<key attr.name=\"invariant\" attr.type=\"string\" for=\"node\" id=\"invariant\"/>\n";
            ret = ret +
                  "<key attr.name=\"invariant.scope\" attr.type=\"string\" for=\"node\" id=\"invariant.scope\"/>\n";
            ret = ret + "<key attr.name=\"sourcecode\" attr.type=\"string\" for=\"edge\" id=\"sourcecode\"/>\n";
            ret = ret + "<key attr.name=\"startline\" attr.type=\"int\" for=\"edge\" id=\"startline\"/>\n";
            ret = ret + "<key attr.name=\"startoffset\" attr.type=\"int\" for=\"edge\" id=\"startoffset\"/>\n";
            ret = ret + "<key attr.name=\"control\" attr.type=\"string\" for=\"edge\" id=\"control\"/>\n";
            ret = ret + "<key attr.name=\"assumption\" attr.type=\"string\" for=\"edge\" id=\"assumption\"/>\n";
            ret = ret +
                  "<key attr.name=\"assumption.scope\" attr.type=\"string\" for=\"edge\" id=\"assumption.scope\"/>\n";
            ret = ret + "<graph edgedefault=\"directed\">\n";
            ret = ret + getGraphmlStr("data", taskInfo["witness-type"], vector<string>({"key", "witness-type"})) + "\n";
            ret = ret + getGraphmlStr("data", taskInfo["sourcecodelang"], vector<string>({"key", "sourcecodelang"})) + "\n";
            ret = ret + getGraphmlStr("data", taskInfo["producer"], vector<string>({"key", "producer"})) + "\n";
            ret = ret + getGraphmlStr("data", taskInfo["specification"], vector<string>({"key", "specification"})) + "\n";
            ret = ret + getGraphmlStr("data", taskInfo["programfile"], vector<string>({"key", "programfile"})) + "\n";
            ret = ret + getGraphmlStr("data", fileHash, vector<string>({"key", "programhash"})) + "\n";
            ret = ret + getGraphmlStr("data", taskInfo["memorymodel"], vector<string>({"key", "memorymodel"})) + "\n";
            ret = ret + getGraphmlStr("data", taskInfo["architecture"], vector<string>({"key", "architecture"})) + "\n";

            int nodeNo = 0;
            for (wg::Node *node : nodes) {
                if (node->name == "") {
                    node->name = "N" + to_string(nodeNo);
                    ++nodeNo;
                }

                string content = "";
                if (node->isEntry) {
                    content = content + getGraphmlStr("data", "true", vector<string>({"key", "entry"})) + "\n";
                }
                if (node->isViolation) {
                    content = content + getGraphmlStr("data", "true", vector<string>({"key", "violation"})) + "\n";
                }
                if (node->invariant != "") {
                    content = content + getGraphmlStr("data", node->invariant, vector<string>({"key", "invariant"})) + "\n";
                    content = content +
                              getGraphmlStr("data", node->invariantScope, vector<string>({"key", "invariant.scope"})) + "\n";
                }

                ret = ret + getGraphmlStr("node", content, vector<string>({"id", node->name})) + "\n";
            }

            for (wg::Edge *edge : edges) {
                string content = "";
                content = content + getGraphmlStr("data", edge->srcCode, vector<string>({"key", "sourcecode"})) + "\n";
                content = content +
                          getGraphmlStr("data", to_string(edge->line), vector<string>({"key", "startline"})) + "\n";
                if (edge->control > 0) {
                    content = content +
                              getGraphmlStr("data", "condition-true", vector<string>({"key", "control"})) + "\n";
                } else if (edge->control < 0) {
                    content = content +
                              getGraphmlStr("data", "condition-false", vector<string>({"key", "control"})) + "\n";
                }
                if (edge->assumption != "") {
                    content = content +
                              getGraphmlStr("data", edge->assumption, vector<string>({"key", "assumption"})) + "\n";
                    content = content +
                              getGraphmlStr("data", edge->assumptionScope, vector<string>({"key", "assumption.scope"})) + "\n";
                }

                ret = ret + getGraphmlStr("edge", content,
                                          vector<string>({"source", edge->start->name, "target", edge->end->name})) + "\n";
            }

            ret = ret + "</graph>" + "\n";
            ret = ret + "</graphml>" + "\n";
            return ret;
        }
    };

}

#endif

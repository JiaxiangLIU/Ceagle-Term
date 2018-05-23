#ifndef SV_CEAGLE_MAXIMALLY_SHARED_GRAPH_H
#define SV_CEAGLE_MAXIMALLY_SHARED_GRAPH_H

#include <vector>
#include <string>
#include <ostream>

namespace ceagle {

    enum MSGVarType {
        BOOL = 0, INT, BV, REAL, FLOAT
    };
    static const std::string MSGVarName[] = {"BOOL", "INT", "BV", "REAL", "FLOAT"};

    /**
     * typical length: bool(1), char(8), int(32), float(32), double(64)
     * length is number of bits, -1 means error, 0 means it's an constant
     */
    class MSGExpressionType {
    public:
        MSGVarType type;
        std::string typeLength;

        static MSGExpressionType makeInt32Type() {
            return MSGExpressionType(MSGVarType::INT, "32");
        }

        static MSGExpressionType makeBoolType() {
            return MSGExpressionType(MSGVarType::BOOL, "1");
        }

        static MSGExpressionType makeCharType() {
            return MSGExpressionType(MSGVarType::INT, "8");
        }

        static MSGExpressionType makeFloatType() {
            return MSGExpressionType(MSGVarType::REAL, "32");
        }

        static MSGExpressionType makeDoubleType() {
            return MSGExpressionType(MSGVarType::REAL, "64");
        }

        static MSGExpressionType getCommonType(MSGExpressionType l, MSGExpressionType r) {
            if (l.isHigherThan(&r)) {
                return l;
            } else {
                return r;
            }
        }

        MSGExpressionType() {}

        MSGExpressionType(MSGVarType p, std::string l) : type(p), typeLength(l) {}

        bool isHigherThan(MSGExpressionType *other) {
            if (this->type > other->type) {
                return true;
            } else if (this->type < other->type) {
                return false;
            }

            int length = (typeLength != "") ? std::stoi(typeLength) : 0;
            int otherLength = (other->typeLength != "") ? std::stoi(other->typeLength) : 0;

            if (length > otherLength) {
                return true;
            } else {
                return false;
            }
        }

        std::string toString() {
            return MSGVarName[type] + "" + typeLength;
        }
    };

    enum MSGOperator {
        EQU = 1,
        DIS_EQU,
        LESS,
        GREAT,
        LESS_EQU,
        GREAT_EQU,
        ADD,
        MINUS,
        MULTIPLY,
        DIVIDE,
        // propositional
                AND,
        XOR,
        OR,
        NOT,
        IMPLICATION,
        // bitvector
                BV_NOT,
        BW_OR,
        BW_AND,
        BW_XOR,
        // ite
                ITE
    };
    static const std::string MSGOperatorName[] = {"", "EQU", "DIS_EQU", "LESS", "GREAT", "LESS_EQU", "GREAT_EQU", "ADD",
                                                  "MINUS", "MULTIPLY", "DIVIDE", "AND", "XOR", "OR", "NOT",
                                                  "IMPLICATION", "BV_NOT", "BW_OR", "BW_AND", "BW_XOR", "ITE"};

    /**
    * base msg node
    */
    class MSGNode {
    public:
        std::vector<int> parentsIndex;
        bool hasVisited = false; // mark visited nodes when refining to avoid re-visiting nodes
        bool hasTransformed = false; // mark node that had been transformed to smt expr
        std::string smtValue = "";
        int index; // index in vector of MSG,it will be convenient that smt model value to msg nodes

        // DW: TODO, why isn't it a pointer?!
        MSGExpressionType it; // intrinsic type(it)

        virtual bool isLeafNode() {
            return false;
        }

        virtual bool isVCNode() {
            return false;
        }

        std::string toString() {
            std::string result = "<";
            result += "" + std::to_string(index) + ">[";
            std::string pIds = "pi:";
            for (std::vector<int>::iterator iter = this->parentsIndex.begin();
                 iter != this->parentsIndex.end(); ++iter) {
                if (iter + 1 == parentsIndex.end()) {
                    pIds += std::to_string(*iter);
                    int a = 0;
                } else {
                    pIds += (std::to_string(*iter) + ",");
                }
            }
            result += (pIds + ",");
            if (hasVisited) {
                result += ("visited,");
            } else {
                result += ("not-visited,");
            }
            result += ("smt:" + smtValue + ",");
            result += it.toString();
            result += "]";
            return result;
        }
    };

    class MSGSummaryNode : public MSGNode {
    public:
        bool isRetValue;
        std::string funcName;
        std::string varName;

        bool isLeafNode() {
            return true;
        }

        std::string toString() {
            std::string result;
            if (isRetValue) {
                result = "[MSGSummaryNode " + funcName + ".ret " + MSGNode::toString() + "]";
            } else {
                result = "[MSGSummaryNode " + funcName + "." + varName + " " + MSGNode::toString() + "]";
            }

            return result;
        }
    };

    /**
     * op node
     */
    class MSGOPNode : public MSGNode {
    public:
        MSGOperator msgOperator;
        MSGExpressionType childtt; // children nodes intrinsic type(it)

        MSGOperator getMSGOperator() const {
            return msgOperator;
        }

        std::string toString() {
            std::string result = "";
            result += (MSGOperatorName[msgOperator] + "-");
            result += (childtt.toString() + " ");
            result += MSGNode::toString();
            return result;
        }
    };

    /**
    * verification condition(VC) node
    * this kind msg node is used to represented the VC,and can be transformed to smt formula sovled by z3
    */
    class MSGVCNode : public MSGOPNode {
    public:
        MSGNode *leftNode;
        MSGNode *rightNode;

        bool isVCNode() {
            return true;
        }

        std::string toString() {
            std::string result = MSGNode::toString();
            result += (" VCNode ");
            return result;
        }
    };

    /**
     * unary node
     */
    class MSGUnaryOPNode : public MSGOPNode {
    public:
        MSGNode *operatorNode;

        std::string toString() {
            std::string result = "[UnaryOPNode " + MSGOPNode::toString() + "]";
            return result;
        }
    };

    /**
     * binary node
     */
    class MSGBinaryOPNode : public MSGOPNode {
    public:
        MSGNode *left;
        MSGNode *right;

        std::string toString() {
            std::string result = "[BinaryOPNode " + MSGOPNode::toString() + "]";
            return result;
        }
    };

    /**
     * if-then-else node
     */
    class MSGITEOPNode : public MSGOPNode {
    public:
        MSGNode *condNode;
        MSGNode *trueNode;
        MSGNode *falseNode;

        std::string toString() {
            std::string result = MSGOPNode::toString() + " ITEOPNode ";
            return result;
        }
    };

    /**
     * variable node
     */
    class MSGVariableNode : public MSGNode {
    public:
        std::string name;

        MSGVariableNode(std::string n) : name(n) {}

        bool isLeafNode() {
            return true;
        }

        std::string toString() {
            std::string result = "[MSGVariableNode " + name + " " + MSGNode::toString() + "]";
            return result;
        }
    };

    /**
     * constant node
     */
    class MSGConstantNode : public MSGNode {
    public:
        std::string value;

        bool isLeafNode() {
            return true;
        }

        MSGConstantNode(std::string constantValue) :
                value(constantValue) {}

        std::string toString() {
            std::string result = "[MSGConstantNode " + value + " " + MSGNode::toString() + "]";
            return result;
        }
    };

    /**
     * main data structure for structural abstraction(maximally-shared graph)
     */
    // DW: this class is not well designed, a little hard to use
    class MSG {
    private:
    public:
//        std::vector<MSGNode *> resultNodes; // results of symbolic execution(verification condition nodes and other result nodes)
        std::vector<MSGNode *> vcNodes; // verification condition nodes
        std::vector<MSGNode *> leafNodes; // auxiliary nodes
        std::vector<MSGNode *> nonLeafNodes; // auxiliary nodes
        std::vector<MSGNode *> allNodes; // all nodes in the msg,they were used for smt model value to nodes

        std::string toString();

        // DW: functions
        void addNode(MSGNode *node, bool isVCNode = false);

//        const std::vector<MSGNode *> &getResultNodes() const;

        const std::vector<MSGNode *> &getVcNodes() const;

        const std::vector<MSGNode *> &getLeafNodes() const;

        const std::vector<MSGNode *> &getNonLeafNodes() const;

        std::vector<MSGNode *> &getAllNodes();

        /**
         * construct msg info recursively
         *
         * @param node current node
         * @param result string used for recording msg
         * @param depth current node depth from root of msg
         */
        void constructMSGInfo(MSGNode *node, std::string &result, int depth);
    };
}


#endif //SV_CEAGLE_MAXIMALLY_SHARED_GRAPH_H

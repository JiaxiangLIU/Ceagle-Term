#include <iostream>
#include "ViolationWitness.h"

#include "llvm/IR/DebugLoc.h"

using llvm::DebugLoc;

namespace ceagle {

void ViolationWitness::initWithPath(vector<const BasicBlock*> errorPath, map<string, string> assumptions) {
    FILE* srcCodeFile = fopen(this->taskInfo["programfile"].c_str(), "r");
    vector<fpos_t> linePos = vector<fpos_t>();

    char buffer[100000];
    while (1) {
        fpos_t pos;
        fgetpos(srcCodeFile, &pos);
        linePos.push_back(pos);
        if (fgets(buffer, sizeof(buffer), srcCodeFile) == NULL) {
            break;
        }
    }

    Node* node1 = new Node(1, 0);
    Node* node2;
    Edge* edge;

    this->add(node1);
    unsigned lastLine = -1, lastCol = -1;
    for (int i = 0; i < errorPath.size(); ++ i) {
        const BasicBlock* b = errorPath[i];
        for (BasicBlock::const_iterator bi = b->begin(); bi != b->end(); ++ bi) {
            DebugLoc dl = bi->getDebugLoc();
            if (!dl) {
                continue;
            }
            unsigned line = dl.getLine();
            unsigned col = dl.getCol();
            if (!line || !col) {
                continue;
            }
            fpos_t pos = linePos[line - 1];
            fsetpos(srcCodeFile, &pos);
            fgets(buffer, sizeof(buffer), srcCodeFile);
            string code(buffer);
            int c = col - 1;
            -- c;
            while (c >= 0 && code[c] != ';') {
                -- c;
            }
            ++ c;
            while (c < code.length()) {
                if (code[c] == '\t' || code[c] == ' ') {
                    ++ c;
                } else {
                    break;
                }
            }
            if (c == code.length() - 1) {
                continue;
            }
            if (c + 3 <= code.length() && (code.substr(c, c + 3) == "if(" || code.substr(c, c + 3) == "if ")) {
                
            } else if (c + 4 <= code.length() && (code.substr(c, c + 4) == "for(" || code.substr(c, c + 3) == "for ")) {

            } else if (c + 6 <= code.length() && (code.substr(c, c + 6) == "while(" || code.substr(c, c + 6) == "while ")) {

            } else {

            }
        }
    }
    this->init(node1);

    fclose(srcCodeFile);
}

void ViolationWitness::initEmpty() {
    Node* n = new Node(1, 1);
    this->add(n);
}

}

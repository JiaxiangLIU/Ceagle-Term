#ifndef CEAGLE_CDE_H
#define CEAGLE_CDE_H

namespace ceagle {

namespace cde {

enum class CDE_TYPE {
    ECA,
    Elevator
};


int runECA(const char *sourceFile, int timeLimit);
int runElevator(const char *sourceFile, int timeLimit);

class CDE {
    CDE_TYPE type;
public:
    CDE(CDE_TYPE type) : type(type) {}
    int run(const char *sourceFile, int timeLimit) {
        switch (type) {
            case CDE_TYPE::ECA:
                return runECA(sourceFile, timeLimit);
            case CDE_TYPE::Elevator:
                return runElevator(sourceFile, timeLimit);
        }
    }
};

}

}

#endif //CEAGLE_CDE_H

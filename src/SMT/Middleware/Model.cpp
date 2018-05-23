#include "SMT/Middleware/Model.h"

using namespace ceagle::smt;
using namespace std;

const string& Model::operator[](const string& name) {
    return model[name];
}

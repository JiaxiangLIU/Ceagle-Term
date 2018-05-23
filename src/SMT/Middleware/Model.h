#ifndef CEAGLE_SMT_MODEL_H
#define CEAGLE_SMT_MODEL_H
#include <map>
#include <string>
#include "SMT/Middleware/AST.h"
namespace ceagle {

namespace smt {

class Model {
    std::map<std::string, std::string> model;
public:
    Model() : model() { }
    Model(std::map<std::string, std::string> m) : model(std::move(m)) {
    }
    virtual ~Model() = default;
    const std::string& operator[](const std::string& name);
    std::string str() const {
        std::stringstream result;
        for (auto & item: model) {
            result << item.first << ": " << item.second << "\n";
        }
        return result.str();
    }
};

}

}
#endif

#ifndef PROPERTY_MANAGER_H
#define PROPERTY_MANAGER_H 

#include <string>
#include <vector>

namespace ceagle {

enum PropertyType {
    PT_Nothing, 
    PT_SVCOMP_Error_Function_Unreachability, 
    PT_SVCOMP_Valid_Free,
    PT_SVCOMP_Valid_Dereference, 
    PT_SVCOMP_Valid_Memory_Tracking, 
    PT_SVCOMP_No_Overflow, 
    PT_SVCOMP_Termination
};

class PropertyManager {
    std::vector<PropertyType> m_propertiesFound;
public:
	void parse(std::string fn);
    PropertyType getProperty();
    std::string getPropertyString();
};

}

#endif

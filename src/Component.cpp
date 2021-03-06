// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#include "crocore/Component.hpp"

namespace crocore{

Component::Component(const std::string &theName):
m_name(theName)
{
    
}

Component::~Component() 
{
}

PropertyPtr Component::get_property_by_name(const std::string & thePropertyName)
{
    std::list<PropertyPtr>::iterator it = m_propertyList.begin();
    for (; it != m_propertyList.end(); ++it)
    {
        PropertyPtr &property = *it;
        if (property->name() == thePropertyName)
            return property;
    }
    throw PropertyNotFoundException(thePropertyName);
}

const std::list<PropertyPtr>&
Component::get_property_list() const
{
    return m_propertyList;
}

void Component::register_property(PropertyPtr theProperty)
{
    m_propertyList.push_back(theProperty);
}

void Component::unregister_property(PropertyPtr theProperty)
{
    m_propertyList.remove(theProperty);
}
    
void Component::observe_properties(const std::list<PropertyPtr>& theProps,  bool b)
{
    std::list<PropertyPtr>::const_iterator it = theProps.begin();
    for (; it != theProps.end(); ++it)
    {
        const PropertyPtr &property = *it;
        if (b)
            property->add_observer(shared_from_this());
        else
            property->remove_observer(shared_from_this());
    }
}
    
void Component::observe_properties(bool b)
{
    observe_properties(m_propertyList, b);
}
    
void Component::unregister_all_properties()
{
    m_propertyList.clear();
}

bool Component::call_function(const std::string &the_function_name,
                              const std::vector<std::string> &the_params)
{
    auto iter = m_function_map.find(the_function_name);
    
    if(iter != m_function_map.end())
    {
        iter->second(the_params);
        return true;
    }
    return false;
}
    
void Component::register_function(const std::string &the_name, Functor the_functor)
{
    m_function_map[the_name] = the_functor;
}

void Component::unregister_function(const std::string &the_name)
{
    auto iter = m_function_map.find(the_name);
    if(iter != m_function_map.end()){ m_function_map.erase(iter); }
}

void Component::unregister_all_functions()
{
    m_function_map.clear();
}

}
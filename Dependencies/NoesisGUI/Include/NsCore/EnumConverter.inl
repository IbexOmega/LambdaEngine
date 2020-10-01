////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


namespace Noesis
{

////////////////////////////////////////////////////////////////////////////////////////////////////
template<class T>
EnumConverter<T>::EnumConverter(): BaseEnumConverter(TypeOf<T>())
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
template<class T>
Ptr<BaseComponent> EnumConverter<T>::Box(uint32_t v) const
{
    return Boxing::Box((T)v);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
template<class T>
uint32_t EnumConverter<T>::Unbox(BaseComponent* value) const
{
    return (uint32_t)(Boxing::Unbox<T>(value));
}

}
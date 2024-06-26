#include "Registry.hpp"
#include "String.hpp"
#include "SharedPtr.hpp"
#include "Array.hpp"
#include "HashMap.hpp"
#include "Logger.hpp"

namespace Fyrion
{
    namespace
    {
        HashMap<String, Array<SharedPtr<TypeHandler>>>      typesByName{};
        HashMap<TypeID, Array<SharedPtr<TypeHandler>>>      typesByID{};
        HashMap<String, SharedPtr<FunctionHandler>>         functionsByName{};
        HashMap<TypeID, Array<TypeHandler*>>                typesByAttribute{};
        HashMap<TypeID, Array<FunctionHandler*>>            functionsByAttribute{};
        Logger&                                             logger = Logger::GetLogger("Fyrion::Registry");
    }

    ParamHandler::ParamHandler(usize index, const FieldInfo& fieldInfo) : m_fieldInfo(fieldInfo)
    {
        m_name.Append("param_");
        m_name.Append(index);
    }

    const FieldInfo& ParamHandler::GetFieldInfo() const
    {
        return m_fieldInfo;
    }

    const String& ParamHandler::GetName() const
    {
        return m_name;
    }

    void ParamHandler::SetName(const StringView& name)
    {
        m_name = name;
    }

    ValueHandler::ValueHandler(const String& valueDesc) : m_valueDesc(valueDesc)
    {
    }

    StringView ValueHandler::GetDesc() const
    {
        return m_valueDesc;
    }

    ConstPtr ValueHandler::GetValue() const
    {
        if (m_fnGetValue)
        {
            return m_fnGetValue(this);
        }
        return nullptr;
    }

    i64 ValueHandler::GetCode() const
    {
        if (m_fnGetCode)
        {
            return m_fnGetCode(this);
        }
        return I64_MIN;
    }

    bool ValueHandler::Compare(ConstPtr value) const
    {
        if (m_fnCompare)
        {
            return m_fnCompare(this, value);
        }
        return false;
    }

    ConstPtr AttributeHandler::GetAttribute(TypeID attributeId) const
    {
        if (auto it = m_attributes.Find(attributeId))
        {
            if (it->second->GetValue)
            {
                return it->second->GetValue(it->second.Get());
            }
        }
        return nullptr;
    }

    bool AttributeHandler::HasAttribute(TypeID attributeId) const
    {
        return m_attributes.Find(attributeId) != m_attributes.end();
    }

    Span<AttributeInfo*> AttributeHandler::GetAttributes() const
    {
        return m_attributeArray;
    }

    ConstructorHandler::ConstructorHandler(FieldInfo* params, usize paramsCount)
    {
        for (int i = 0; i < paramsCount; ++i)
        {
            m_params.EmplaceBack(i, params[i]);
        }
    }

    VoidPtr ConstructorHandler::NewInstance(Allocator& allocator, VoidPtr* params)
    {
        if (m_newInstanceFn)
        {
            return m_newInstanceFn(this, allocator, params);
        }
        return nullptr;
    }

    void ConstructorHandler::Construct(VoidPtr memory, VoidPtr* params)
    {
        if (m_placementNewFn)
        {
            m_placementNewFn(this, memory, params);
        }
    }

    FieldHandler::FieldHandler(const String& name) : m_name(name)
    {

    }

    StringView FieldHandler::GetName() const
    {
        return m_name;
    }

    FieldInfo FieldHandler::GetFieldInfo() const
    {
        if (m_fnGetFieldInfo)
        {
            return m_fnGetFieldInfo(this);
        }
        return {};
    }

    VoidPtr FieldHandler::GetFieldPointer(VoidPtr instance) const
    {
        if (m_fnGetFieldPointer)
        {
            return m_fnGetFieldPointer(this, instance);
        }
        return nullptr;
    }

    void FieldHandler::CopyValueTo(ConstPtr instance, VoidPtr value)
    {
        if (m_fnCopyValueTo)
        {
            m_fnCopyValueTo(this, instance, value);
        }
    }

    void FieldHandler::SetValue(VoidPtr instance, ConstPtr value)
    {
        if (m_fnSetValue)
        {
            m_fnSetValue(this, instance, value);
        }
    }


    StringView FunctionHandler::GetName() const
    {
        return m_name;
    }

    StringView FunctionHandler::GetSimpleName() const
    {
        return m_simpleName;
    }

    Span<ParamHandler> FunctionHandler::GetParams() const
    {
        return m_params;
    }

    FieldInfo FunctionHandler::GetReturn() const
    {
        return m_return;
    }

    TypeID FunctionHandler::GetOwner() const
    {
        return m_owner;
    }

    VoidPtr FunctionHandler::GetFunctionPointer() const
    {
        return m_functionPointer;
    }

    void FunctionHandler::Invoke(VoidPtr instance, VoidPtr ret, VoidPtr* params) const
    {
        if (m_fnInvoke)
        {
            m_fnInvoke(this, instance, ret, params);
        }
    }

    void FunctionHandler::OnAttributeCreated(TypeID attributeId)
    {
        auto fIt = functionsByAttribute.Find(attributeId);
        if (fIt == functionsByAttribute.end())
        {
            fIt = functionsByAttribute.Emplace(attributeId, Array<FunctionHandler*>{}).first;
        }
        fIt->second.EmplaceBack(this);
    }

    TypeHandler::TypeHandler(const StringView& name, const TypeInfo& typeInfo, u32 version) : m_name(name), m_typeInfo(typeInfo), m_version(version)
    {
        m_simpleName = Fyrion::GetSimpleName(name);
    }

    ConstructorHandler* TypeHandler::FindConstructor(TypeID* ids, usize size) const
    {
        u64 constructorId = size > 0 ? MurmurHash64(ids, size * sizeof(TypeID), HashSeed64) : 0;
        if (auto it = m_constructors.Find(constructorId))
        {
            return it->second.Get();
        }
        return nullptr;
    }

    Span<ConstructorHandler*> TypeHandler::GetConstructors() const
    {
        return m_constructorArray;
    }

    FieldHandler* TypeHandler::FindField(const StringView& fieldName) const
    {
        if (auto it = m_fields.Find(fieldName))
        {
            return it->second.Get();
        }
        return nullptr;
    }

    Span<FieldHandler*> TypeHandler::GetFields() const
    {
        return m_fieldArray;
    }

    FunctionHandler* TypeHandler::FindFunction(const StringView& functionName) const
    {
        if (auto it = m_functions.Find(functionName))
        {
            return it->second.Get();
        }
        return nullptr;
    }

    Span<FunctionHandler*> TypeHandler::GetFunctions() const
    {
        return m_functionArray;
    }

    ValueHandler* TypeHandler::FindValueByName(const StringView& valueName) const
    {
        if(auto it = m_values.Find(valueName))
        {
            return it->second.Get();
        }
        return nullptr;
    }

    ValueHandler* TypeHandler::FindValueByCode(i64 code) const
    {
        if(auto it = m_valuesByCode.Find(code))
        {
            return it->second;
        }
        return nullptr;
    }

    Span<ValueHandler*> TypeHandler::GetValues() const
    {
        return m_valuesArray;
    }

    Span<DerivedType> TypeHandler::GetDerivedTypes() const
    {
        return m_derivedTypes;
    }

    Array<TypeID> TypeHandler::GetBaseTypes() const
    {
        return m_baseTypesArray;
    }

    StringView TypeHandler::GetName() const
    {
        return m_name;
    }

    StringView TypeHandler::GetSimpleName() const
    {
        return m_simpleName;
    }

    const TypeInfo& TypeHandler::GetTypeInfo() const
    {
        return m_typeInfo;
    }

    u32 TypeHandler::GetVersion() const
    {
        return m_version;
    }

    void TypeHandler::Destroy(VoidPtr instance, Allocator& allocator) const
    {
        if (m_fnDestroy)
        {
            m_fnDestroy(this, allocator, instance);
        }
    }

    void TypeHandler::Release(VoidPtr instance) const
    {
        if (m_fnRelease)
        {
            m_fnRelease(this, instance);
        }
    }

    void TypeHandler::Destructor(VoidPtr instance) const
    {
        if (m_fnDestructor)
        {
            m_fnDestructor(this, instance);
        }
    }

    void TypeHandler::Copy(ConstPtr source, VoidPtr dest) const
    {
        if (m_fnCopy)
        {
            m_fnCopy(this, source, dest);
        }
    }

    void TypeHandler::Move(VoidPtr source, VoidPtr dest) const
    {
        if (m_fnMove)
        {
            m_fnMove(this, source, dest);
        }
    }

    VoidPtr TypeHandler::Cast(TypeID typeId, VoidPtr instance) const
    {
        if (typeId == m_typeInfo.typeId)
        {
            return instance;
        }
        if (auto it = m_baseTypes.Find(typeId))
        {
            return it->second(this, instance);
        }
        return instance;
    }

    void TypeHandler::OnAttributeCreated(TypeID attributeId)
    {
        auto fIt = typesByAttribute.Find(attributeId);
        if (fIt == typesByAttribute.end())
        {
            fIt = typesByAttribute.Emplace(attributeId, Array<TypeHandler*>{}).first;
        }
        fIt->second.EmplaceBack(this);
    }

    ///builders

    AttributeBuilder::AttributeBuilder(AttributeHandler& attributeHandler) : m_attributeHandler(attributeHandler)
    {
    }

    AttributeInfo& AttributeBuilder::NewAttribute(TypeID attributeId)
    {
        auto it = m_attributeHandler.m_attributes.Find(attributeId);
        if (it == m_attributeHandler.m_attributes.end())
        {
            it =m_attributeHandler.m_attributes.Emplace(attributeId, MakeShared<AttributeInfo>()).first;
            m_attributeHandler.m_attributeArray.EmplaceBack(it->second.Get());
            m_attributeHandler.OnAttributeCreated(attributeId);
        }
        return *it->second;
    }

    ValueBuilder::ValueBuilder(ValueHandler& valueHandler) : valueHandler(valueHandler)
    {
    }

    void ValueBuilder::SetFnGetValue(ValueHandler::FnGetValue fnGetValue)
    {
        valueHandler.m_fnGetValue = fnGetValue;
    }

    void ValueBuilder::SerFnGetCode(ValueHandler::FnGetCode fnGetCode)
    {
        valueHandler.m_fnGetCode = fnGetCode;
    }

    void ValueBuilder::SetFnCompare(ValueHandler::FnCompare fnCompare)
    {

        valueHandler.m_fnCompare = fnCompare;
    }

    ConstructorBuilder::ConstructorBuilder(ConstructorHandler& constructorHandler) : m_constructorHandler(constructorHandler)
    {
    }

    void ConstructorBuilder::SetPlacementNewFn(ConstructorHandler::PlacementNewFn placementNew)
    {
        m_constructorHandler.m_placementNewFn = placementNew;
    }

    void ConstructorBuilder::SetNewInstanceFn(ConstructorHandler::NewInstanceFn newInstance)
    {
        m_constructorHandler.m_newInstanceFn = newInstance;
    }

    FieldBuilder::FieldBuilder(FieldHandler& fieldHandler) : m_fieldHandler(fieldHandler)
    {
    }

    FieldHandler& FieldBuilder::GetFieldHandler() const
    {
        return m_fieldHandler;
    }

    void FieldBuilder::SetFnGetFieldInfo(FieldHandler::FnGetFieldInfo fnGetFieldInfo)
    {
        m_fieldHandler.m_fnGetFieldInfo = fnGetFieldInfo;
    }

    void FieldBuilder::SetFnGetFieldPointer(FieldHandler::FnGetFieldPointer fnGetFieldPointer)
    {
        m_fieldHandler.m_fnGetFieldPointer = fnGetFieldPointer;
    }

    void FieldBuilder::SetFnCopyValueTo(FieldHandler::FnCopyValueTo fnCopyValueTo)
    {
        m_fieldHandler.m_fnCopyValueTo = fnCopyValueTo;
    }

    void FieldBuilder::SetFnSetValue(FieldHandler::FnSetValue fnSetValue)
    {
        m_fieldHandler.m_fnSetValue = fnSetValue;
    }

    FunctionBuilder::FunctionBuilder(FunctionHandler& functionHandler) : m_functionHandler(functionHandler)
    {
    }

    void FunctionBuilder::Create(const FunctionHandlerCreation& creation)
    {
        m_functionHandler.m_params.Clear();
        m_functionHandler.m_name = creation.name;
        m_functionHandler.m_functionId = creation.functionId;
        m_functionHandler.m_return = creation.retInfo;
        m_functionHandler.m_owner = creation.owner;

        m_functionHandler.m_simpleName = GetSimpleName(m_functionHandler.m_name);

        for (int i = 0; i < creation.params.Size(); ++i)
        {
            m_functionHandler.m_params.EmplaceBack(i, creation.params[i]);
        }
    }

    FunctionHandler& FunctionBuilder::GetFunctionHandler() const
    {
        return m_functionHandler;
    }

    ParamHandler& FunctionBuilder::GetParam(usize index)
    {
        return m_functionHandler.m_params[index];
    }

    void FunctionBuilder::SetFnInvoke(FunctionHandler::FnInvoke fnInvoke)
    {
        m_functionHandler.m_fnInvoke = fnInvoke;
    }

    void FunctionBuilder::SetFunctionPointer(VoidPtr functionPointer)
    {
        m_functionHandler.m_functionPointer = functionPointer;
    }

    TypeBuilder::TypeBuilder(TypeHandler& typeHandler) :  m_typeHandler(typeHandler)
    {

    }

    void TypeBuilder::SetFnDestroy(TypeHandler::FnDestroy fnDestroy)
    {
        m_typeHandler.m_fnDestroy = fnDestroy;
    }

    void TypeBuilder::SetFnCopy(TypeHandler::FnCopy fnCopy)
    {
        m_typeHandler.m_fnCopy = fnCopy;
    }

    void TypeBuilder::SetFnDestructor(TypeHandler::FnDestructor destructor)
    {
        m_typeHandler.m_fnDestructor = destructor;
    }

    void TypeBuilder::SetFnRelease(TypeHandler::FnRelease fnRelease)
    {
        m_typeHandler.m_fnRelease = fnRelease;
    }

    void TypeBuilder::SetFnMove(TypeHandler::FnMove fnMove)
    {
        m_typeHandler.m_fnMove = fnMove;
    }

    ConstructorBuilder TypeBuilder::NewConstructor(TypeID* ids, FieldInfo* params, usize size)
    {
        u64 constructorId = size > 0 ? MurmurHash64(ids, size * sizeof(TypeID), HashSeed64) : 0;
        auto it = m_typeHandler.m_constructors.Find(constructorId);

        if (it == m_typeHandler.m_constructors.end())
        {
            it = m_typeHandler.m_constructors.Emplace(constructorId, MakeShared<ConstructorHandler>(params, size)).first;
            m_typeHandler.m_constructorArray.EmplaceBack(it->second.Get());
        }
        return *it->second;
    }

    FieldBuilder TypeBuilder::NewField(const StringView& fieldName)
    {
        auto it = m_typeHandler.m_fields.Find(fieldName);
        if (it == m_typeHandler.m_fields.end())
        {
            it = m_typeHandler.m_fields.Emplace(fieldName, MakeShared<FieldHandler>(fieldName)).first;
            m_typeHandler.m_fieldArray.EmplaceBack(it->second.Get());
        }
        return FieldBuilder{*it->second};
    }

    ValueBuilder TypeBuilder::NewValue(const StringView& valueDesc, i64 code)
    {
        auto it = m_typeHandler.m_values.Find(valueDesc);
        if (it == m_typeHandler.m_values.end())
        {
            it = m_typeHandler.m_values.Emplace(valueDesc, MakeShared<ValueHandler>(valueDesc)).first;
            m_typeHandler.m_valuesArray.EmplaceBack(it->second.Get());
            m_typeHandler.m_valuesByCode.Emplace(code, it->second.Get());
        }
        return ValueBuilder{*it->second};
    }

    FunctionBuilder TypeBuilder::NewFunction(const FunctionHandlerCreation& creation)
    {
        auto it = m_typeHandler.m_functions.Find(creation.name);
        if (it == m_typeHandler.m_functions.end())
        {
            it = m_typeHandler.m_functions.Emplace(String{creation.name}, MakeShared<FunctionHandler>()).first;
            m_typeHandler.m_functionArray.EmplaceBack(it->second.Get());
        }

        FunctionBuilder builder{*it->second};
        builder.Create(creation);
        return builder;
    }

    void TypeBuilder::AddBaseType(TypeID typeId, FnCast fnCast)
    {
        TypeHandler* baseType = Registry::FindTypeById(typeId);
        FY_ASSERT(baseType, "Base Type not found");

        baseType->m_derivedTypes.EmplaceBack(m_typeHandler.GetTypeInfo().typeId, fnCast);
        m_typeHandler.m_baseTypes.Insert(typeId, fnCast);
        m_typeHandler.m_baseTypesArray.EmplaceBack(typeId);
    }

    TypeHandler& TypeBuilder::GetTypeHandler() const
    {
        return m_typeHandler;
    }


    TypeBuilder Registry::NewType(const StringView& name, const TypeInfo& typeInfo)
    {
        auto itByName = typesByName.Find(name);
        if (!itByName)
        {
            itByName = typesByName.Emplace(name, Array<SharedPtr<TypeHandler>>{}).first;
        }

        auto itById = typesByID.Find(typeInfo.typeId);
        if (!itById)
        {
            itById = typesByID.Emplace(typeInfo.typeId, Array<SharedPtr<TypeHandler>>{}).first;
        }

        usize version = itByName->second.Size() + 1;
        SharedPtr<TypeHandler> typeHandler = MakeShared<TypeHandler>(name, typeInfo, version);

        itByName->second.EmplaceBack(typeHandler);
        itById->second.EmplaceBack(typeHandler);

        logger.Debug("Type {} Registered ", name, version);

        return {*typeHandler};
    }

    FunctionBuilder Registry::NewFunction(const FunctionHandlerCreation& creation)
    {
        auto it = functionsByName.Find(creation.name);
        if (it == functionsByName.end())
        {
            it = functionsByName.Emplace(String{creation.name}, MakeShared<FunctionHandler>()).first;
        }

        FunctionBuilder builder{*it->second};
        builder.Create(creation);
        return builder;
    }

    FunctionHandler* Registry::FindFunctionByName(const StringView& name)
    {
        if (auto it = functionsByName.Find(name))
        {
            return it->second.Get();
        }
        return nullptr;
    }

    Span<FunctionHandler*> Registry::FindFunctionsByAttribute(TypeID typeId)
    {
        auto it = functionsByAttribute.Find(typeId);
        if (it != functionsByAttribute.end())
        {
            return it->second;
        }
        return {};
    }

    TypeHandler* Registry::FindTypeByName(const StringView& name)
    {
        if (auto it = typesByName.Find(name))
        {
            return it->second.Back().Get();
        }
        return nullptr;
    }

    TypeHandler* Registry::FindTypeById(TypeID typeId)
    {
        if (auto it = typesByID.Find(typeId))
        {
            return it->second.Back().Get();
        }
        return nullptr;
    }

    Span<TypeHandler*> Registry::FindTypesByAttribute(TypeID typeId)
    {
        if (auto it = typesByAttribute.Find(typeId))
        {
            return it->second;
        }
        return {};
    }

    void RegistryShutdown()
    {
        typesByName.Clear();
        typesByID.Clear();
        functionsByName.Clear();
        typesByAttribute.Clear();
        functionsByAttribute.Clear();
    }
}
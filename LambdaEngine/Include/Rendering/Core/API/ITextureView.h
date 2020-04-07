#pragma once
#include "IDeviceChild.h"
#include "GraphicsTypes.h"

namespace LambdaEngine
{
    class ITexture;

    enum class ETextureViewType : uint8
    {
        TEXTURE_VIEW_NONE   = 0,
        TEXTURE_VIEW_1D     = 1,
        TEXTURE_VIEW_2D     = 2,
        TEXTURE_VIEW_3D     = 3,
        TEXTURE_VIEW_CUBE   = 4,
    };

    enum FTextureViewFlags : uint32
    {
        TEXTURE_VIEW_FLAG_NONE              = 0,
        TEXTURE_VIEW_FLAG_RENDER_TARGET     = 1,
        TEXTURE_VIEW_FLAG_DEPTH_STENCIL     = 2,
        TEXTURE_VIEW_FLAG_UNORDERED_ACCESS  = 3,
        TEXTURE_VIEW_FLAG_SHADER_RESOURCE   = 4,
    };
    
    struct TextureViewDesc
    {
        const char*         pName           = "";
        ITexture*           pTexture        = nullptr;
        uint32              Flags           = FTextureViewFlags::TEXTURE_VIEW_FLAG_NONE;
        EFormat             Format          = EFormat::NONE;
        ETextureViewType    Type            = ETextureViewType::TEXTURE_VIEW_NONE;
        uint32              MiplevelCount   = 0;
        uint32              ArrayCount      = 0;
        uint32              Miplevel       = 0;
        uint32              ArrayIndex      = 0;
    };

    class ITextureView : public IDeviceChild
    {
    public:
        DECL_DEVICE_INTERFACE(ITextureView);
        
        virtual ITexture*       GetTexture()      = 0;
        virtual uint64          GetHandle() const = 0;
        virtual TextureViewDesc GetDesc()   const = 0;
    };
}
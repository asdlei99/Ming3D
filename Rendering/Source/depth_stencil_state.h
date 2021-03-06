#ifndef MING3D_DEPTHSTENCILSTATE_H
#define MING3D_DEPTHSTENCILSTATE_H

namespace Ming3D
{
    enum class DepthStencilDepthFunc
    {
        Less, Equal, Greater, LEqual, GEqual
    };

    struct DepthStencilStateDesc
    {
        bool mDepthEnabled = true;
        DepthStencilDepthFunc mDepthFunc = DepthStencilDepthFunc::LEqual;
    };

    class DepthStencilState
    {
    };
}

#endif

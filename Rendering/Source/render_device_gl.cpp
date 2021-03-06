#ifdef MING3D_OPENGL
#include "render_device_gl.h"

#include "vertex_buffer_gl.h"
#include "index_buffer_gl.h"
#include "render_target_gl.h"
#include "shader_program_gl.h"
#include "texture_buffer_gl.h"
#include "constant_buffer_gl.h"

#include "Debug/debug.h"
#include "Debug/st_assert.h"
#include "shader_writer_glsl.h"
#include "Debug/debug_stats.h"

namespace Ming3D
{
    RenderDeviceGL::RenderDeviceGL()
    {
        const GLubyte* vendor = glGetString(GL_VENDOR);
        const GLubyte* renderer = glGetString(GL_RENDERER);
        LOG_INFO() << "Graphics Vendor: " << vendor;
        LOG_INFO() << "Graphics Renderer: " << renderer;

        if (glewInit())
        {
            LOG_ERROR() << "Failed to initialise GLEW";
        }

        mDefaultRasteriserState = (RasteriserStateGL*)CreateRasteriserState(RasteriserStateCullMode::Back, true);
        
        DepthStencilStateDesc dssDesc;
        dssDesc.mDepthFunc = DepthStencilDepthFunc::Less;
        dssDesc.mDepthEnabled = true;
        mDefaultDepthStencilState = (DepthStencilStateGL*)CreateDepthStencilState(dssDesc);

        SetRasteriserState(mDefaultRasteriserState);
        SetDepthStencilState(mDefaultDepthStencilState);
    }

    RenderDeviceGL::~RenderDeviceGL()
    {

    }

    RenderTarget* RenderDeviceGL::CreateRenderTarget(RenderWindow* inWindow)
    {
        TextureInfo textureInfo;
        textureInfo.mWidth = inWindow->GetWindow()->GetWidth();
        textureInfo.mHeight = inWindow->GetWindow()->GetHeight();

        RenderTargetGL* renderTarget = (RenderTargetGL*)CreateRenderTarget(textureInfo, 1);
        renderTarget->mWindowTarget = true;
        return renderTarget;
    }

    RenderTarget* RenderDeviceGL::CreateRenderTarget(TextureInfo inTextureInfo, int numTextures)
    {
        RenderTargetGL* renderTarget = new RenderTargetGL();

        GLuint FramebufferName = 0;
        glGenFramebuffers(1, &FramebufferName);
        glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);

        TextureBufferGL* colourBuffer = new TextureBufferGL();

        for (int i = 0; i < numTextures; i++)
        {
            GLuint renderTexture;
            glGenTextures(1, &renderTexture);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, renderTexture);

            glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, inTextureInfo.mWidth, inTextureInfo.mHeight);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, renderTexture, 0);

            renderTarget->mAttachments.push_back(GL_COLOR_ATTACHMENT0 + i);

            colourBuffer->SetGLTexture(renderTexture);
            renderTarget->mColourBuffers.push_back(colourBuffer);
        }

        renderTarget->mFrameBufferID = FramebufferName;

        GLuint depthrenderbuffer;
        glGenRenderbuffers(1, &depthrenderbuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, depthrenderbuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, inTextureInfo.mWidth, inTextureInfo.mHeight);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthrenderbuffer);

        TextureBufferGL* depthBuffer = new TextureBufferGL();
        depthBuffer->SetGLTexture(depthrenderbuffer);
        renderTarget->mDepthRenderBuffer = depthBuffer;

        return renderTarget;
    }

    VertexBuffer* RenderDeviceGL::CreateVertexBuffer(VertexData* inVertexData)
    {
        VertexBufferGL* vertexBuffer = new VertexBufferGL();
        GLuint vbo;
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, inVertexData->GetNumVertices() * inVertexData->GetVertexSize(), inVertexData->GetDataPtr(), GL_STATIC_DRAW);
        vertexBuffer->SetGLBuffer(vbo);
        vertexBuffer->SetVertexLayout(inVertexData->GetVertexLayout());
        return vertexBuffer;
    }

    IndexBuffer* RenderDeviceGL::CreateIndexBuffer(IndexData* inIndexData)
    {
        IndexBufferGL* indexBuffer = new IndexBufferGL();
        GLuint ibo;
        glGenBuffers(1, &ibo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, inIndexData->GetNumIndices() * sizeof(unsigned int), inIndexData->GetData(), GL_STATIC_DRAW);
        indexBuffer->SetGLBuffer(ibo);
        indexBuffer->SetNumIndices(inIndexData->GetNumIndices());
        return indexBuffer;
    }

    ShaderProgram* RenderDeviceGL::CreateShaderProgram(ParsedShaderProgram* parsedProgram)
    {
        ShaderWriterGLSL shaderWriter;

        ConvertedShaderProgramGLSL* convertedProgram = static_cast<ConvertedShaderProgramGLSL*>(parsedProgram->mConvertedProgram);
        if (convertedProgram == nullptr)
        {
            ShaderProgramDataGLSL convertedShaderData;
            if (!shaderWriter.WriteShader(parsedProgram, convertedShaderData))
                return nullptr;
            convertedProgram = new ConvertedShaderProgramGLSL();
            convertedProgram->mShaderProgramData = convertedShaderData;
            parsedProgram->mConvertedProgram = convertedProgram;
        }

        ShaderProgramDataGLSL convertedShaderData = convertedProgram->mShaderProgramData;

        GLuint program = glCreateProgram();
        GLuint vs = glCreateShader(GL_VERTEX_SHADER);
        GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);

        const char* str_v = convertedShaderData.mVertexShader.mSource.c_str();
        const char* str_f = convertedShaderData.mFragmentShader.mSource.c_str();

        glShaderSource(vs, 1, &str_v, 0);
        glShaderSource(fs, 1, &str_f, 0);

        glCompileShader(vs);
        glCompileShader(fs);
        glAttachShader(program, vs);
        glAttachShader(program, fs);
        glLinkProgram(program);

        int vstatus, fstatus, lstatus;
        glGetShaderiv(vs, GL_COMPILE_STATUS, &vstatus);
        printf("Vertex shader compile status: %s\n", (vstatus == GL_TRUE) ? "true" : "false");
        glGetShaderiv(fs, GL_COMPILE_STATUS, &fstatus);
        printf("Fragment shader compile status: %s\n", (fstatus == GL_TRUE) ? "true" : "false");
        glGetProgramiv(program, GL_LINK_STATUS, &lstatus);
        printf("Program link status: %s\n", (lstatus == GL_TRUE) ? "true" : "false");
            
        if (vstatus == GL_FALSE)
        {
            GLchar infoLog[1024];
            glGetShaderInfoLog(vs, sizeof(infoLog), NULL, infoLog);
            LOG_ERROR() << "Error compling vertex shader: " << std::string(infoLog);
        }
        else if (fstatus == GL_FALSE)
        {
            GLchar infoLog[1024];
            glGetShaderInfoLog(fs, sizeof(infoLog), NULL, infoLog);
            LOG_ERROR() << "Error compling fragment shader: " << std::string(infoLog);
        }
        else if (lstatus == GL_FALSE)
        {
            GLchar infoLog[1024];
            glGetProgramInfoLog(program, sizeof(infoLog), NULL, infoLog);
            LOG_ERROR() << "Error linking shader program: " << std::string(infoLog);
        }

        ShaderProgramGL* shaderProgram = new ShaderProgramGL();
        shaderProgram->SetGLProgram(program);
        shaderProgram->SetGLVertexShader(vs);
        shaderProgram->SetGLFragmentShader(fs);

        return shaderProgram;
    }

    TextureBuffer* RenderDeviceGL::CreateTextureBuffer(TextureInfo inTextureInfo, void* inTextureData)
    {
        ADD_DEBUG_STAT_INT("CreateTextureBuffer", 1);

        TextureBufferGL* textureBuffer = new TextureBufferGL();

        __Assert(inTextureData); // TODO: Clear if null
        
        char* buffer = new char[inTextureInfo.mWidth * inTextureInfo.mHeight * inTextureInfo.mBytesPerPixel];
        // Flip the texture
        for (size_t i = 0; i < inTextureInfo.mHeight; i++)
        {
            const size_t rowSize = inTextureInfo.mWidth * inTextureInfo.mBytesPerPixel;
            memcpy(buffer + (rowSize * (inTextureInfo.mHeight - 1)) - (rowSize * i), (char*)inTextureData + rowSize * i, rowSize);
        }

        GLuint glTexture;
        glGenTextures(1, &glTexture);
        glBindTexture(GL_TEXTURE_2D, glTexture);

        GLint pixelFormat;
        if (inTextureInfo.mPixelFormat == PixelFormat::RGB)
            pixelFormat = GL_RGB;
        else if (inTextureInfo.mPixelFormat == PixelFormat::BGRA)
            pixelFormat = GL_BGRA;
        else
            pixelFormat = GL_RGBA;
        GLint internalFormat = (inTextureInfo.mPixelFormat == PixelFormat::RGB) ? GL_RGB : GL_RGBA;

        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, inTextureInfo.mWidth, inTextureInfo.mHeight, 0, pixelFormat, GL_UNSIGNED_BYTE, buffer);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        textureBuffer->SetGLTexture(glTexture);

        delete[] buffer;

        return textureBuffer;
    }

    RenderWindow* RenderDeviceGL::CreateRenderWindow(WindowBase* inWindow)
    {
        RenderWindowGL* renderWindow = new RenderWindowGL(inWindow);

        return renderWindow;
    }

    RasteriserState* RenderDeviceGL::CreateRasteriserState(RasteriserStateCullMode inCullMode, bool inDepthClipEnabled)
    {
        RasteriserStateGL* rasteriserState = new RasteriserStateGL();
        rasteriserState->mCullMode = inCullMode;
        rasteriserState->mDepthClipEnabled = inDepthClipEnabled;
        return rasteriserState;
    }

    DepthStencilState* RenderDeviceGL::CreateDepthStencilState(DepthStencilStateDesc inDesc)
    {
        DepthStencilStateGL* depthStencilState = new DepthStencilStateGL();
        switch (inDesc.mDepthFunc)
        {
        case DepthStencilDepthFunc::Less:
            depthStencilState->mDepthFunc = GL_LESS;
            break;
        case DepthStencilDepthFunc::LEqual:
            depthStencilState->mDepthFunc = GL_LEQUAL;
            break;
        case DepthStencilDepthFunc::Equal:
            depthStencilState->mDepthFunc = GL_EQUAL;
            break;
        case DepthStencilDepthFunc::GEqual:
            depthStencilState->mDepthFunc = GL_LEQUAL;
            break;
        case DepthStencilDepthFunc::Greater:
            depthStencilState->mDepthFunc = GL_GREATER;
            break;
        }
        // TODO: enable/disable depth?
        return depthStencilState;
    }

    ConstantBuffer* RenderDeviceGL::CreateConstantBuffer(size_t inSize)
    {
        ConstantBufferGL* cb = new ConstantBufferGL();

        GLuint ubo = 0;
        char* initialData = new char[inSize];
        glGenBuffers(1, &ubo);
        glBindBuffer(GL_UNIFORM_BUFFER, ubo);
        glBufferData(GL_UNIFORM_BUFFER, inSize, initialData, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
        delete[] initialData;
        cb->mGLBuffer = ubo;

        return cb;
    }

    void RenderDeviceGL::SetTexture(const TextureBuffer* inTexture, int inSlot)
    {
        ADD_FRAME_STAT_INT("SetTexture", 1);

        glEnable(GL_TEXTURE_2D); // TODO
        TextureBufferGL* glTexture = (TextureBufferGL*)inTexture;
        glActiveTexture(GL_TEXTURE0 + inSlot);
        glBindTexture(GL_TEXTURE_2D, glTexture->GetGLTexture());
    }

    void RenderDeviceGL::SetActiveShaderProgram(ShaderProgram* inProgram)
    {
        ADD_FRAME_STAT_INT("SetActiveShaderProgram", 1);

        mActiveShaderProgram = (ShaderProgramGL*)inProgram;
        if (mActiveShaderProgram != nullptr)
        {
            glUseProgram(mActiveShaderProgram->GetGLProgram());
        }
    }

    void RenderDeviceGL::BeginRenderWindow(RenderWindow* inWindow)
    {
        mRenderWindow = (RenderWindowGL*)inWindow;

        mRenderWindow->GetWindow()->BeginRender();
    }

    void RenderDeviceGL::EndRenderWindow(RenderWindow* inWindow)
    {
        __Assert(mRenderWindow == inWindow);

        mRenderWindow->GetWindow()->EndRender();
        mRenderWindow = nullptr;
    }

    void RenderDeviceGL::BeginRenderTarget(RenderTarget* inTarget)
    {
        mRenderTarget = (RenderTargetGL*)inTarget;

        glBindFramebuffer(GL_FRAMEBUFFER, mRenderTarget->mFrameBufferID);
        glDrawBuffers(1, mRenderTarget->mAttachments.data());

        glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glClear(GL_DEPTH_BUFFER_BIT);

        //glEnable(GL_DEPTH_TEST);
        //glDepthFunc(GL_LEQUAL);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    void RenderDeviceGL::EndRenderTarget(RenderTarget* inTarget)
    {
        __Assert(mRenderTarget == inTarget);

        RenderTargetGL* glTarget = (RenderTargetGL*)inTarget;

        // If rendering to window render target, blit framebuffers (from render target FBO to Window's default FBO)
        if (glTarget->mWindowTarget)
        {
            BlitRenderTarget(glTarget, mRenderWindow);
        }

        mRenderTarget = nullptr;
    }

    void RenderDeviceGL::BlitRenderTarget(RenderTargetGL* inSourceTarget, RenderWindow* inTargetWindow)
    {
        const int w = inTargetWindow->GetWindow()->GetWidth();
        const int h = inTargetWindow->GetWindow()->GetHeight();
        glBindFramebuffer(GL_READ_FRAMEBUFFER, inSourceTarget->mFrameBufferID);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        glViewport(0, 0, w, h);
        glBlitFramebuffer(0, 0, w, h, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }

    void RenderDeviceGL::RenderPrimitive(VertexBuffer* inVertexBuffer, IndexBuffer* inIndexBuffer)
    {
        ADD_FRAME_STAT_INT("RenderPrimitive", 1);

        VertexBufferGL* vertexBufferGL = (VertexBufferGL*)inVertexBuffer;
        IndexBufferGL* indexBufferGL = (IndexBufferGL*)inIndexBuffer;
        
        size_t vertexComponentIndex = 0;
        size_t vertexComponentOffset = 0;
        for (EVertexComponent vertexComponent : inVertexBuffer->GetVertexLayout().VertexComponents)
        {
            const size_t vertexComponentSize = VertexData::GetVertexComponentSize(vertexComponent);
            glEnableVertexAttribArray(vertexComponentIndex);
            glBindBuffer(GL_ARRAY_BUFFER, vertexBufferGL->GetGLBuffer());
            glVertexAttribPointer(vertexComponentIndex, VertexData::GetVertexComponentSize(vertexComponent) / sizeof(float), GL_FLOAT, GL_FALSE, vertexBufferGL->GetVertexSize(), (void*)vertexComponentOffset);
            vertexComponentIndex++;
            vertexComponentOffset += vertexComponentSize;
        }

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferGL->GetGLBuffer());
        glDrawElements(GL_TRIANGLES, indexBufferGL->GetNumIndices(), GL_UNSIGNED_INT, 0);
    }

    void RenderDeviceGL::SetRasteriserState(RasteriserState* inState)
    {
        if (inState == nullptr)
            inState = mDefaultRasteriserState;

        RasteriserStateGL* glRasterState = (RasteriserStateGL*)inState;
        
        if (!glRasterState->mDepthClipEnabled || glRasterState->mCullMode == RasteriserStateCullMode::None)
        {
            glDisable(GL_CULL_FACE);
        }
        else
        {
            glEnable(GL_CULL_FACE);
            glCullFace(glRasterState->mCullMode == RasteriserStateCullMode::Front ? GL_FRONT : GL_BACK);
            glFrontFace(GL_CCW);
        }
    }

    void RenderDeviceGL::SetDepthStencilState(DepthStencilState* inState)
    {
        DepthStencilStateGL* glStencilState = (DepthStencilStateGL*)inState;
        mDefaultDepthStencilState = glStencilState;

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(glStencilState->mDepthFunc);
    }

    void RenderDeviceGL::SetConstantBufferData(ConstantBuffer* inConstantBuffer, void* inData, size_t inSize)
    {
        ConstantBufferGL* cb = static_cast<ConstantBufferGL*>(inConstantBuffer);

        glBindBuffer(GL_UNIFORM_BUFFER, cb->mGLBuffer);
        GLvoid* ptr = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
        memcpy(ptr, inData, inSize);
        glUnmapBuffer(GL_UNIFORM_BUFFER);
    }

    void RenderDeviceGL::BindConstantBuffer(ConstantBuffer* inConstantBuffer, const char* inName, ShaderProgram* inProgram)
    {
        ConstantBufferGL* cb = static_cast<ConstantBufferGL*>(inConstantBuffer);
        ShaderProgramGL* prog = static_cast<ShaderProgramGL*>(inProgram);

        // get index
        unsigned int block_index = glGetUniformBlockIndex(prog->GetGLProgram(), inName);
        // bind UBO to shader program
        glBindBufferBase(GL_UNIFORM_BUFFER, block_index, cb->mGLBuffer);
    }

    void RenderDeviceGL::SetShaderUniformFloat(const std::string& inName, float inVal)
    {
        ADD_FRAME_STAT_INT("SetConstantBufferData", 1);

        GLuint loc = mActiveShaderProgram->GetUniformLocation(inName);
        glUniform1f(loc, inVal);
    }

    void RenderDeviceGL::SetShaderUniformInt(const std::string& inName, int inVal)
    {
        ADD_FRAME_STAT_INT("SetConstantBufferData", 1);

        GLuint loc = mActiveShaderProgram->GetUniformLocation(inName);
        glUniform1i(loc, inVal);
    }

    void RenderDeviceGL::SetShaderUniformMat4x4(const std::string& inName, const glm::mat4 inMat)
    {
        ADD_FRAME_STAT_INT("SetConstantBufferData", 1);

        GLuint loc = mActiveShaderProgram->GetUniformLocation(inName);
        glUniformMatrix4fv(loc, 1, GL_FALSE, &inMat[0][0]);
    }

    void RenderDeviceGL::SetShaderUniformVec2(const std::string& inName, const glm::vec2 inVec)
    {
        ADD_FRAME_STAT_INT("SetConstantBufferData", 1);

        GLuint loc = mActiveShaderProgram->GetUniformLocation(inName);
        glUniform2fv(loc, 1, (float*)&inVec[0]);
    }

    void RenderDeviceGL::SetShaderUniformVec3(const std::string& inName, const glm::vec3 inVec)
    {
        ADD_FRAME_STAT_INT("SetConstantBufferData", 1);

        GLuint loc = mActiveShaderProgram->GetUniformLocation(inName);
        glUniform3fv(loc, 1, (float*)&inVec[0]);
    }

    void RenderDeviceGL::SetShaderUniformVec4(const std::string& inName, const glm::vec4 inVec)
    {
        ADD_FRAME_STAT_INT("SetConstantBufferData", 1);

        GLuint loc = mActiveShaderProgram->GetUniformLocation(inName);
        glUniform4fv(loc, 1, (float*)&inVec[0]);
    }
}
#endif

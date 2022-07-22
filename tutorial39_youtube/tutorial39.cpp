/*

        Copyright 2022 Etay Meiri

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    Toon Shading & Rim Lighting
*/

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <GL/glew.h>


#include "ogldev_engine_common.h"
#include "ogldev_util.h"
#include "ogldev_basic_glfw_camera.h"
#include "ogldev_new_lighting.h"
#include "ogldev_glfw.h"
#include "ogldev_basic_mesh.h"
#include "ogldev_world_transform.h"
#include "ogldev_phong_renderer.h"

#define WINDOW_WIDTH  1920
#define WINDOW_HEIGHT 1080

static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
static void CursorPosCallback(GLFWwindow* window, double x, double y);
static void MouseButtonCallback(GLFWwindow* window, int Button, int Action, int Mode);


class Tutorial39
{
public:

    Tutorial39()
    {
        m_dirLight.WorldDirection = Vector3f(1.0f, -1.0f, 0.0f);
        m_dirLight.DiffuseIntensity = 1.0f;
        m_dirLight.AmbientIntensity = 0.1f;
    }

    virtual ~Tutorial39()
    {
        SAFE_DELETE(m_pGameCamera);
    }


    void Init()
    {
        CreateWindow();

        InitCallbacks();

        InitCamera();

        InitMesh();

        InitRenderer();
    }


    void Run()
    {
        while (!glfwWindowShouldClose(window)) {
            RenderSceneCB();
            glfwSwapBuffers(window);
            glfwPollEvents();
        }
    }


    void RenderSceneCB()
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        m_pGameCamera->OnRender();

        static float foo = 0.0f;
        foo += 0.002f;

        //        m_pGameCamera->SetPosition(-sinf(foo) * 13.0f, 8.0f, -cosf(foo) * 13.0f);
        //        Vector3f Target(m_pMesh1->GetPosition() - m_pGameCamera->GetPos() + Vector3f(0.0f, 3.0f, 0.0f));
        //        m_pGameCamera->SetTarget(Target);

        m_dirLight.WorldDirection = Vector3f(sinf(foo), -0.5f, cosf(foo));
        m_phongRenderer.UpdateDirLightDir(m_dirLight.WorldDirection);

        if (m_fogEnabled) {
            m_phongRenderer.SetLinearFog(m_fogStart, m_fogEnd, m_fogColor);
        } else {
            m_phongRenderer.SetLinearFog(-1.0f, -1.0f, Vector3f(0.0f, 0.0f, 0.0f));
        }

        m_phongRenderer.Render(m_pTerrain);
    }


#define ATTEN_STEP 0.01f

#define ANGLE_STEP 1.0f

    void PassiveMouseCB(int x, int y)
    {
        m_pGameCamera->OnMouse(x, y);
    }

    void KeyboardCB(uint key, int state)
    {
        if (state == GLFW_PRESS) {
            switch (key) {
            case GLFW_KEY_ESCAPE:
            case GLFW_KEY_Q:
                glfwDestroyWindow(window);
                glfwTerminate();
                exit(0);

            case GLFW_KEY_F:
                m_fogEnabled = !m_fogEnabled;
            }
        }

        m_pGameCamera->OnKeyboard(key);
    }


    void MouseCB(int button, int action, int x, int y)
    {
    }


private:

    void CreateWindow()
    {
        int major_ver = 0;
        int minor_ver = 0;
        bool is_full_screen = false;
        window = glfw_init(major_ver, minor_ver, WINDOW_WIDTH, WINDOW_HEIGHT, is_full_screen, "Tutorial 39");

        glfwSetCursorPos(window, WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2);
    }


    void InitCallbacks()
    {
        glfwSetKeyCallback(window, KeyCallback);
        glfwSetCursorPosCallback(window, CursorPosCallback);
        glfwSetMouseButtonCallback(window, MouseButtonCallback);
    }


    void InitCamera()
    {
        Vector3f Pos(0.0f, 0.0f, 0.0f);
        Vector3f Target(0.0f, 0.0f, 1.0f);
        Vector3f Up(0.0, 1.0f, 0.0f);

        float FOV = 45.0f;
        float zNear = 0.1f;
        float zFar = 100.0f;
        PersProjInfo persProjInfo = { FOV, (float)WINDOW_WIDTH, (float)WINDOW_HEIGHT, zNear, zFar };

        m_pGameCamera = new BasicCamera(persProjInfo, Pos, Target, Up);
    }


    void InitRenderer()
    {
        m_phongRenderer.InitPhongRenderer();
        m_phongRenderer.SetCamera(m_pGameCamera);
        m_phongRenderer.SetDirLight(m_dirLight);
        m_phongRenderer.SetLinearFog(m_fogStart, m_fogEnd, m_fogColor);
    }


    void InitMesh()
    {
        m_pTerrain = new BasicMesh();
        m_pTerrain->LoadMesh("../Content/terrain2.obj");
        m_pTerrain->SetPosition(0.0f, 0.0f, 0.0f);
    }

    GLFWwindow* window = NULL;
    BasicCamera* m_pGameCamera = NULL;
    PhongRenderer m_phongRenderer;
    BasicMesh* m_pTerrain = NULL;
    PersProjInfo m_persProjInfo;
    DirectionalLight m_dirLight;
    float m_fogStart = 5.0f;
    float m_fogEnd = 100.0f;
    Vector3f m_fogColor = Vector3f(152.0f/256.0f, 152.0f/256.0f, 152.0f/256.0f);
    bool m_fogEnabled = true;
};

Tutorial39* app = NULL;

static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    app->KeyboardCB(key, action);
}


static void CursorPosCallback(GLFWwindow* window, double x, double y)
{
    app->PassiveMouseCB((int)x, (int)y);
}


static void MouseButtonCallback(GLFWwindow* window, int Button, int Action, int Mode)
{
    double x, y;

    glfwGetCursorPos(window, &x, &y);

    app->MouseCB(Button, Action, (int)x, (int)y);
}


int main(int argc, char** argv)
{
    app = new Tutorial39();

    app->Init();

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glFrontFace(GL_CW);
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    app->Run();

    delete app;

    return 0;
}

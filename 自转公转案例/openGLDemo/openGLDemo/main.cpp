//
//  main.cpp
//  openGLDemo
//
//  Created by 李江 on 2020/7/3.
//  Copyright © 2020 李江. All rights reserved.
//

#include "GLTools.h"
#include "GLShaderManager.h"
#include "GLFrustum.h"
#include "GLBatch.h"
#include "GLMatrixStack.h"
#include "GLGeometryTransform.h"
#include "StopWatch.h"

#include <math.h>
#include <stdio.h>

#ifdef __APPLE__
#include <glut/glut.h>
#else
#define FREEGLUT_STATIC
#include <GL/glut.h>
#endif

/*
 GLMatrixStack 变化管线使用矩阵堆栈
 
 GLMatrixStack 构造函数允许指定堆栈的最大深度、默认的堆栈深度为64.这个矩阵堆在初始化时已经在堆栈中包含了单位矩阵。
 GLMatrixStack::GLMatrixStack(int iStackDepth = 64);
 
 //通过调用顶部载入这个单位矩阵
 void GLMatrixStack::LoadIndentiy(void);
 
 //在堆栈顶部载入任何矩阵
 void GLMatrixStack::LoadMatrix(const M3DMatrix44f m);
 */
////设置角色帧，作为相机
GLFrame             viewFrame;
//使用GLFrustum类来设置透视投影
//001--综合训练(地板)
GLShaderManager        shaderManager;            // 着色器管理器
GLMatrixStack        modelViewMatrix;        // 模型视图矩阵
GLMatrixStack        projectionMatrix;        // 投影矩阵
GLFrustum            viewFrustum;            // 视景体
GLGeometryTransform    transformPipeline;        // 几何图形变换管道

GLTriangleBatch        torusBatch;             //大球
GLTriangleBatch     sphereBatch;            //小球
GLBatch                floorBatch;          //地板


//角色帧 照相机角色帧
GLFrame             cameraFrame;

//**4、添加附加随机球
#define NUM_SPHERES 50
GLFrame spheres[NUM_SPHERES];

// 此函数在呈现上下文中进行任何必要的初始化。.
// 这是第一次做任何与opengl相关的任务。
void SetupRC()
{
    //1.初始化背景
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    //初始化着色器管理器
    shaderManager.InitializeStockShaders();
    
    //2.开启深度测试
    glEnable(GL_DEPTH_TEST);
    
    //3.设置地板的顶点数据 设置顶点通过线条连接 数量324
    floorBatch.Begin(GL_LINES, 324);
    for(GLfloat x = -20.0; x <= 20.0f; x+= 0.5) {
        floorBatch.Vertex3f(x, -0.55f, 20.0f);
        floorBatch.Vertex3f(x, -0.55f, -20.0f);
        
        floorBatch.Vertex3f(20.0f, -0.55f, x);
        floorBatch.Vertex3f(-20.0f, -0.55f, x);
    }
    floorBatch.End();
    
    //4.设置大球模型
    //参数1：球批次类
    //参数2：半径
    //参数3：三角形数量
    //参数4：三角形数量
    gltMakeSphere(torusBatch, 0.4f, 40, 80);
    
    
    //5.设置小球球模型
    gltMakeSphere(sphereBatch, 0.1f, 30, 60);
    //随机位置放置小球球,因为小球数量很多，所以随机放置小球
    for (int i = 0; i < NUM_SPHERES; i++) {
        
        //y轴不变，X,Z产生随机值
        GLfloat x = ((GLfloat)((rand() % 400) - 200 ) * 0.1f);
        GLfloat z = ((GLfloat)((rand() % 400) - 200 ) * 0.1f);
        
        //在y方向，将球体设置为0.0的位置，这使得它们看起来是飘浮在眼睛的高度
        //对spheres数组中的每一个顶点，设置顶点数据
        spheres[i].SetOrigin(x, 0.0f, z);
    }
    
}

//渲染场景
void RenderScene()
{
    
    //清楚颜色缓存区和深度缓存区
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    //压栈 ->  进行多个操作 单元矩阵 -> 旋转 -> 平移 -> 缩放 -> 出栈
    modelViewMatrix.PushMatrix();
    
    //加入观察者 平移10步(地板,大球,小球,小小球)
    M3DMatrix44f mCamera;
    cameraFrame.GetCameraMatrix(mCamera);
    modelViewMatrix.PushMatrix(mCamera);
    
    ///---------  首先绘制地板 --------------------
    //1.颜色值(地板,大球,小球颜色)
    static GLfloat vFloorColor[] = { 0.0f, 1.0f, 0.0f, 1.0f};
    //2.绘制地板
    shaderManager.UseStockShader(GLT_SHADER_FLAT,transformPipeline.GetModelViewProjectionMatrix(),vFloorColor);
    //3.开始绘制
    floorBatch.Draw();
    
    /// -------------- 绘制大球 ----------------
    //1.大球颜色值(地板,大球,小球颜色)
    static GLfloat vTorusColor[] = { 1.0f, 0.0f, 0.0f, 1.0f };
    //2.基于时间动画，因为大球要自转所以要加定时器
    static CStopWatch    rotTimer;
    float yRot = rotTimer.GetElapsedSeconds() * 60.0f;
    //3.获取光源位置
    M3DVector4f vLightPos = {0.0f,10.0f,5.0f,1.0f};
    //4.使得大球位置平移(3.0)向屏幕里面
    modelViewMatrix.Translate(0.0f, 0.0f, -3.0f);
    //5.再次压栈(复制栈顶)，是因为自转要一直自转，而平移只会一次，所以要再次压栈
    modelViewMatrix.PushMatrix();
    //6.大球自转
    modelViewMatrix.Rotate(yRot, 0.0f, 1.0f, 0.0f);
    //7.指定合适的着色器(点光源着色器)
    shaderManager.UseStockShader(GLT_SHADER_POINT_LIGHT_DIFF, transformPipeline.GetModelViewMatrix(),
                                 transformPipeline.GetProjectionMatrix(), vLightPos, vTorusColor);
    torusBatch.Draw();
    //8.绘制完毕则Pop
    modelViewMatrix.PopMatrix();
    
    //-------------- 绘制小球 -------------
    //1.大球颜色值(地板,大球,小球颜色)
    static GLfloat vSphereColor[] = { 0.0f, 0.0f, 1.0f, 1.0f};
    //画多个小球小球
    for (int i = 0; i < NUM_SPHERES; i++) {
        modelViewMatrix.PushMatrix();
        //获取新的位置
        modelViewMatrix.MultMatrix(spheres[i]);
        shaderManager.UseStockShader(GLT_SHADER_POINT_LIGHT_DIFF, transformPipeline.GetModelViewMatrix(),
                                     transformPipeline.GetProjectionMatrix(), vLightPos, vSphereColor);
        sphereBatch.Draw();
        modelViewMatrix.PopMatrix();
        
    }
    //2. 让一个小篮球围绕大球公众自转
    modelViewMatrix.Rotate(yRot * -2.0f, 0.0f, 1.0f, 0.0f);
    modelViewMatrix.Translate(0.8f, 0.0f, 0.0f);
    shaderManager.UseStockShader(GLT_SHADER_POINT_LIGHT_DIFF,transformPipeline.GetModelViewMatrix(),transformPipeline.GetProjectionMatrix(),vLightPos,vSphereColor);
    sphereBatch.Draw();
    
    
    modelViewMatrix.PopMatrix();
    modelViewMatrix.PopMatrix();
    //执行缓存区交换
    glutSwapBuffers();
    //重新渲染
    glutPostRedisplay();
    
}

//特殊键位处理（上、下、左、右移动） 进行旋转
void SpecialKeys(int key, int x, int y)
{
    
    float linear = 0.1f;
    //旋转度数
    float angular = float(m3dDegToRad(5.0f));
    
    if (key == GLUT_KEY_UP) {
        //MoveForward 平移
        cameraFrame.MoveForward(linear);
    }
    if (key == GLUT_KEY_DOWN) {
        cameraFrame.MoveForward(-linear);
    }
    
    if (key == GLUT_KEY_LEFT) {
        //RotateWorld 旋转
        cameraFrame.RotateWorld(angular, 0.0f, 1.0f, 0.0f);
    }
    
    if (key == GLUT_KEY_RIGHT) {
        cameraFrame.RotateWorld(-angular, 0.0f, 1.0f, 0.0f);
    }
}





// 窗口已更改大小，或刚刚创建。无论哪种情况，我们都需要
// 使用窗口维度设置视口和投影矩阵.
void ChangeSize(int nWidth, int nHeight)
{
    //1.防止h变为0
    if(nHeight == 0)
        nHeight = 1;
    
    //2.设置视口窗口尺寸
    glViewport(0, 0, nWidth, nHeight);
    
    //3.创建投影矩阵，并将它载入投影矩阵堆栈中
    //参数1：垂直方向上的视场角度
    //参数2：视口纵横比 = w/h
    //参数3：近裁剪面距离
    //参数4：远裁剪面距离
    viewFrustum.SetPerspective(35.0f, float(nWidth) / float(nHeight), 1.0f, 100.0f);
    
    //4.把透视矩阵加载到透视矩阵对阵中
    projectionMatrix.LoadMatrix(viewFrustum.GetProjectionMatrix());
    
    
    //5.设置变换管道以使用两个矩阵堆栈（变换矩阵modelViewMatrix ，投影矩阵projectionMatrix）
    //初始化GLGeometryTransform 的实例transformPipeline.通过将它的内部指针设置为模型视图矩阵堆栈 和 投影矩阵堆栈实例，来完成初始化
    //当然这个操作也可以在SetupRC 函数中完成，但是在窗口大小改变时或者窗口创建时设置它们并没有坏处。而且这样可以一次性完成矩阵和管线的设置。
    transformPipeline.SetMatrixStacks(modelViewMatrix, projectionMatrix);
}

int main(int argc,char* argv[])
{
    //设置当前工作目录，针对MAC OS X
    gltSetWorkingDirectory(argv[0]);
    //初始化GLUT库
    glutInit(&argc, argv);
    //申请一个颜色缓存区、深度缓存区、双缓存区、模板缓存区
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_STENCIL);
    //设置window 的尺寸
    glutInitWindowSize(800, 600);
    //创建window的名称
    glutCreateWindow("OpenGL SphereWorld");
    //注册回调函数（改变尺寸）
    glutReshapeFunc(ChangeSize);
    //特殊键位函数（上下左右）
    glutSpecialFunc(SpecialKeys);
    //显示函数
    glutDisplayFunc(RenderScene);
    
    //判断一下是否能初始化glew库，确保项目能正常使用OpenGL 框架
    GLenum err = glewInit();
    if (GLEW_OK != err) {
        fprintf(stderr, "GLEW Error: %s\n", glewGetErrorString(err));
        return 1;
    }
    
    //绘制
    SetupRC();
    
    //runloop运行循环
    glutMainLoop();
    return 0;
}

#ifndef QHoneycombWidget_H
#define QHoneycombWidget_H


// QT includes
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QMatrix4x4>

// opencl includes
#include "cl.h"
#include "cl_ext.h"
#include "cl_gl.h"

// honeycomb parameters
#define m_NbCell 300
#define m_NbCell 300
#define m_Rayon 20
#define m_SquarredRadius 20.*20.
#define m_InvSquarredRadius (1./(m_SquarredRadius))
#define m_NbCoul 25


class QHoneycombWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    QHoneycombWidget(QWidget *parent = 0);
    ~QHoneycombWidget();

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

    // introduce random noise
    void Randomize();

public slots:
    void cleanup();
    void StartStop() { m_RunKernel = !m_RunKernel; }

protected:
  void initializeGL() override;
  void paintGL() override;
  void resizeGL(int width, int height) override;
    
private:

  // methods
  void SetArray2D(const int i, const int j, const int iVal) { m_Tab2D[i * m_NbCell + j] = iVal; }
  int GetArray2D(const int i, const int j) const { return m_Tab2D[i * m_NbCell + j]; }

  // inits
  cl_context CreateContext();
  cl_command_queue CreateCommandQueue(cl_context context, cl_device_id *device);
  cl_program CreateProgram(cl_device_id device, const char* fileName);
  bool CreateMemObjects();

  void initTexture();
  void InitTab2D();

  // apply iteration
  cl_int computeTexture();
  void QHoneycombWidget::displayTexture(int w, int h);
  
  // fields

  // cpu array storing pixel colors
  std::vector<int> m_Tab2D;

  GLuint tex ;
  int imWidth ;
  int imHeight ;

  // opencl data
  cl_kernel m_KernelCopyIntoTexture /*transform integer into rgba color*/, m_KernelIteration /*compute new color from previous data*/, m_KernelCopyIntoTextureBasic /**/;
  cl_mem cl_tex_mem;
  cl_context clcontext ;
  cl_command_queue commandQueue ;
  cl_program program ;

  cl_mem m_GPUTab2D; /*2D Array of integers that represent colors*/
  cl_mem m_GPUTabNbPixelByColor, m_GPUTabNbPixelByColorInitial;
  cl_mem m_OutputBuffer /**/;

  bool m_RunKernel;
    
};

#endif //QHoneycombWidget_H

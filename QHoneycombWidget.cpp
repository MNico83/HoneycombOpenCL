#include "QHoneycombWidget.h"
#include <QMouseEvent>
#include <QOpenGLShaderProgram>
#include <QCoreApplication>
#include <math.h>

#include <iostream>
#include <fstream>
#include <sstream>

#include <gl/GLU.h>

const int DefaultimWidth = 300;
const int DefaultimHeight = 300;
const int DefaultWindowsSize = 500;

std::string GetPath(const std::string & iFileName)
{
  auto PosPrev = 0, PosCur = 0;
  do
  {
    PosPrev = PosCur;
    PosCur = iFileName.find("\\", PosCur + 1);

  } while (PosCur != std::string::npos);

  return iFileName.substr(0, PosPrev + 1);
}


QHoneycombWidget::QHoneycombWidget(QWidget *parent)
: QOpenGLWidget(parent)
{
  imWidth = DefaultimWidth;
  imHeight = DefaultimHeight;
  //
  m_RunKernel = false;

  tex = 0;
  m_KernelCopyIntoTexture = m_KernelIteration = m_KernelCopyIntoTextureBasic = 0;
  cl_tex_mem = 0;
  clcontext = 0;
  commandQueue = 0;
  program = 0;
}

QHoneycombWidget::~QHoneycombWidget()
{
  cleanup();
}

QSize QHoneycombWidget::minimumSizeHint() const
{
  return QSize(50, 50);
}

QSize QHoneycombWidget::sizeHint() const
{

  return QSize(DefaultWindowsSize, DefaultWindowsSize);
}

void QHoneycombWidget::cleanup()
{
  makeCurrent();

  if (m_GPUTab2D != 0)
    clReleaseMemObject(m_GPUTab2D);
  if (m_GPUTabNbPixelByColor != 0)
    clReleaseMemObject(m_GPUTabNbPixelByColor);
  if (m_GPUTabNbPixelByColorInitial != 0)
    clReleaseMemObject(m_GPUTabNbPixelByColorInitial);
  if (m_OutputBuffer != 0)
    clReleaseMemObject(m_OutputBuffer);

  if (commandQueue != 0)
    clReleaseCommandQueue(commandQueue);
  
  if (clcontext != 0)
    clReleaseContext(clcontext);
  if (m_KernelIteration != 0)
    clReleaseKernel(m_KernelIteration);
  if (m_KernelCopyIntoTexture != 0)
    clReleaseKernel(m_KernelCopyIntoTexture);
  if (m_KernelCopyIntoTextureBasic != 0)
    clReleaseKernel(m_KernelCopyIntoTextureBasic);
  
  if (program != 0)
    clReleaseProgram(program);
  
  if (cl_tex_mem != 0)
    clReleaseMemObject(cl_tex_mem);
  if (tex != 0)
  {
    glBindBuffer(GL_TEXTURE_RECTANGLE_ARB, tex);
    glDeleteBuffers(1, &tex);
  }
  
  
  doneCurrent();
}

cl_context QHoneycombWidget::CreateContext()
{
  cl_int errNum;
  cl_uint numPlatforms;
  cl_platform_id firstPlatformId;
  cl_context context = NULL;

  // First, select an OpenCL platform to run on.  For this example, we
  // simply choose the first available platform.  Normally, you would
  // query for all available platforms and select the most appropriate one.
  errNum = clGetPlatformIDs(1, &firstPlatformId, &numPlatforms);
  if (errNum != CL_SUCCESS || numPlatforms <= 0)
  {
    std::cerr << "Failed to find any OpenCL platforms." << std::endl;
    return NULL;
  }

  // Next, create an OpenCL context on the platform.  Attempt to
  // create a GPU-based context, and if that fails, try to create
  // a CPU-based context.
  cl_context_properties contextProperties[] =
  {
#ifdef _WIN32
    CL_CONTEXT_PLATFORM,
    (cl_context_properties)firstPlatformId,
    CL_GL_CONTEXT_KHR,
    (cl_context_properties)wglGetCurrentContext(),
    CL_WGL_HDC_KHR,
    (cl_context_properties)wglGetCurrentDC(),
#elif defined( __GNUC__)
    CL_CONTEXT_PLATFORM, (cl_context_properties)clSelectedPlatformID,
    CL_GL_CONTEXT_KHR, (cl_context_properties)glXGetCurrentContext(),
    CL_GLX_DISPLAY_KHR, (cl_context_properties)glXGetCurrentDisplay(),
#elif defined(__APPLE__) 
    //todo
#endif
    0



  };
  context = clCreateContextFromType(contextProperties, CL_DEVICE_TYPE_GPU,
    NULL, NULL, &errNum);
  if (errNum != CL_SUCCESS)
  {
    std::cout << "Could not create GPU context, trying CPU..." << std::endl;
    context = clCreateContextFromType(contextProperties, CL_DEVICE_TYPE_CPU,
      NULL, NULL, &errNum);
    if (errNum != CL_SUCCESS)
    {
      std::cerr << "Failed to create an OpenCL GPU or CPU context." << std::endl;
      return NULL;
    }
  }

  return context;
}

cl_command_queue QHoneycombWidget::CreateCommandQueue(cl_context context, cl_device_id *device)
{
  cl_int errNum;
  cl_device_id *devices;
  cl_command_queue commandQueue = NULL;
  size_t deviceBufferSize = -1;

  // First get the size of the devices buffer
  errNum = clGetContextInfo(context, CL_CONTEXT_DEVICES, 0, NULL, &deviceBufferSize);
  if (errNum != CL_SUCCESS)
  {
    std::cerr << "Failed call to clGetContextInfo(...,GL_CONTEXT_DEVICES,...)";
    return NULL;
  }

  if (deviceBufferSize <= 0)
  {
    std::cerr << "No devices available.";
    return NULL;
  }

  // Allocate memory for the devices buffer
  devices = new cl_device_id[deviceBufferSize / sizeof(cl_device_id)];
  errNum = clGetContextInfo(context, CL_CONTEXT_DEVICES, deviceBufferSize, devices, NULL);
  if (errNum != CL_SUCCESS)
  {
    std::cerr << "Failed to get device IDs";
    return NULL;
  }

  // In this example, we just choose the first available device.  In a
  // real program, you would likely use all available devices or choose
  // the highest performance device based on OpenCL device queries
  commandQueue = clCreateCommandQueue(context, devices[0], 0, NULL);
  if (commandQueue == NULL)
  {
    std::cerr << "Failed to create commandQueue for device 0";
    return NULL;
  }

  *device = devices[0];
  delete[] devices;
  return commandQueue;
}



cl_program QHoneycombWidget::CreateProgram(cl_device_id device, const char* fileName)
{
  cl_int errNum;
  cl_program program;

  std::string CLFilePath = GetPath(__FILE__) + "GPGPUHoneycomb.cl";
  std::ifstream kernelFile(CLFilePath, std::ios::in);
  if (!kernelFile.is_open())
  {
    std::cerr << "Failed to open file for reading: " << fileName << std::endl;
    return NULL;
  }

  std::ostringstream oss;
  oss << kernelFile.rdbuf();

  std::string srcStdStr = oss.str();
  const char *srcStr = srcStdStr.c_str();
  program = clCreateProgramWithSource(clcontext, 1, (const char**)&srcStr, NULL, NULL);
  if (program == NULL)
  {
    std::cerr << "Failed to create CL program from source." << std::endl;
    return NULL;
  }

  std::string ArgParams;
  ArgParams += "-D m_NbCell=300 ";
  ArgParams += "-D m_Rayon=20 ";
  ArgParams += "-D m_SquarredRadius=(20.*20.) ";
  ArgParams += "-D m_NbCoul=25 ";
  ArgParams += "-D m_InvSquarredRadius=(1./(m_SquarredRadius)) ";

  errNum = clBuildProgram(program, 0, NULL, ArgParams.c_str(), NULL, NULL);
  if (errNum != CL_SUCCESS)
  {
    // Determine the reason for the error
    char buildLog[16384];
    clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG,
      sizeof(buildLog), buildLog, NULL);

    std::cerr << "Error in kernel: " << std::endl;
    std::cerr << buildLog;
    clReleaseProgram(program);
    return NULL;
  }

  return program;
}

bool QHoneycombWidget::CreateMemObjects()
{
  cl_int errNum;

  cl_tex_mem = clCreateFromGLTexture2D(clcontext, CL_MEM_READ_WRITE, GL_TEXTURE_RECTANGLE_ARB, 0, tex, &errNum);
  if (errNum != CL_SUCCESS)
  {
    std::cerr << __FUNCTION__ << " : Failed creating memory from GL texture." << std::endl;
    return false;
  }

  m_Tab2D.resize(m_NbCell * m_NbCell);
  std::vector<unsigned int> TabNbPixelByColor(m_NbCell, 0), TabNbPixelByColorInitial(m_NbCell, 0);

  InitTab2D();

  for (int jc = 0; jc < m_NbCoul; jc++)
  {
    TabNbPixelByColor[jc] = 0;
  }
  //
  for (int i = 0; i < m_NbCell; i++)
  {
    for (int j = 0; j < m_NbCell; j++)
    {
      TabNbPixelByColor[GetArray2D(i, j)]++;;
    }
  }


  for (int jc = 0; jc < m_NbCoul; jc++)
  {
    TabNbPixelByColorInitial[jc] = 0;
  }
  //
  for (int i = 0; i < m_NbCell; i++)
  {
    for (int j = 0; j < m_NbCell; j++)
    {
      TabNbPixelByColorInitial[GetArray2D(i, j)]++;;
    }
  }

  cl_int err = 0;
  m_GPUTab2D = clCreateBuffer(clcontext, CL_MEM_READ_WRITE, m_NbCell * m_NbCell * sizeof(cl_int), m_Tab2D.data(), &err);
  m_GPUTabNbPixelByColor = clCreateBuffer(clcontext, CL_MEM_READ_WRITE, m_NbCell * sizeof(cl_int), TabNbPixelByColor.data(), &err);
  m_OutputBuffer = clCreateBuffer(clcontext, CL_MEM_READ_WRITE, m_NbCell * m_NbCell * sizeof(cl_uint2), m_Tab2D.data(), &err);
  m_GPUTabNbPixelByColorInitial = clCreateBuffer(clcontext, CL_MEM_READ_WRITE, m_NbCell * sizeof(cl_int), TabNbPixelByColorInitial.data(), &err);

  errNum = clEnqueueWriteBuffer(commandQueue, m_GPUTab2D, true, 0, m_NbCell * m_NbCell * sizeof(cl_uint), m_Tab2D.data(), 0, NULL, NULL);
  errNum = clEnqueueWriteBuffer(commandQueue, m_GPUTabNbPixelByColor, true, 0, m_NbCell * sizeof(unsigned int), TabNbPixelByColor.data(), 0, NULL, NULL);
  errNum = clEnqueueWriteBuffer(commandQueue, m_GPUTabNbPixelByColorInitial, true, 0, m_NbCell * sizeof(unsigned int), TabNbPixelByColorInitial.data(), 0, NULL, NULL);

  errNum = clFinish(commandQueue);

  return true;
}

void QHoneycombWidget::initTexture()
{
  // make a texture for output
  glGenTextures(1, &tex);              // texture 
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
  glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tex);
  glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA32F_ARB, imWidth, imHeight, 0, GL_LUMINANCE, GL_FLOAT, NULL);
}

void QHoneycombWidget::initializeGL()
{
  cl_device_id device = 0;
  connect(context(), &QOpenGLContext::aboutToBeDestroyed, this, &QHoneycombWidget::cleanup);

  initializeOpenGLFunctions();

  initTexture();

  // Create an OpenCL context on first available platform
  clcontext = CreateContext();
  if (clcontext == NULL)
  {
    std::cerr << "Failed to create OpenCL context." << std::endl;
    return ;
  }

  // Create a command-queue on the first device available
  // on the created context
  commandQueue = CreateCommandQueue(clcontext, &device);
  if (commandQueue == NULL)
  {
    cleanup();
    return ;
  }


  // Create OpenCL program from GLinterop.cl kernel source
  program = CreateProgram(device, "Kelvin.cl");
  if (program == NULL)
  {
    cleanup();
    return;
  }

  // Create OpenCL kernel
  m_KernelCopyIntoTexture = clCreateKernel(program, "copy_texture_kernel", NULL);
  if (m_KernelCopyIntoTexture == NULL)
  {
    std::cerr << "Failed to create kernel" << std::endl;
    cleanup();
    return;
  }

  m_KernelCopyIntoTextureBasic = clCreateKernel(program, "copy_texture_kernel_basic", NULL);
  if (m_KernelCopyIntoTextureBasic == NULL)
  {
    std::cerr << "Failed to create kernel" << std::endl;
    cleanup();
    return;
  }

  m_KernelIteration = clCreateKernel(program, "KelvinIteration", NULL);
  if (m_KernelIteration == NULL)
  {
    std::cerr << "Failed to create kernel" << std::endl;
    cleanup();
    return;
  }

  // Create memory objects that will be used as arguments to
  // kernel
  if (!CreateMemObjects())
  {
    cleanup();
    return ;
  }
}


cl_int QHoneycombWidget::computeTexture()
{

  cl_int errNum = 0;

  size_t tex_globalWorkSize[2] = { imWidth, imHeight };
  
  glFinish();
  errNum = clEnqueueAcquireGLObjects(commandQueue, 1, &cl_tex_mem, 0, NULL, NULL);


  if (m_RunKernel)
  {
    int err = 0;
    err = clSetKernelArg(m_KernelIteration, 0, sizeof(cl_mem), (void*)&m_GPUTabNbPixelByColorInitial);
    err = clSetKernelArg(m_KernelIteration, 1, sizeof(cl_mem), (void*)&m_GPUTabNbPixelByColor);
    err = clSetKernelArg(m_KernelIteration, 2, sizeof(cl_mem), (void*)&m_GPUTab2D);
    err = clSetKernelArg(m_KernelIteration, 3, sizeof(cl_mem), (void*)&m_OutputBuffer);


    size_t GridSize[2] = { m_NbCell, m_NbCell };
    err = clEnqueueNDRangeKernel(commandQueue, m_KernelIteration, 2, 0, GridSize, 0, 0, NULL, NULL);

    ///////////////////////////////////////////////////////////
    errNum = clSetKernelArg(m_KernelCopyIntoTexture, 0, sizeof(cl_mem), &cl_tex_mem);
    errNum = clSetKernelArg(m_KernelCopyIntoTexture, 1, sizeof(cl_int), &imWidth);
    errNum = clSetKernelArg(m_KernelCopyIntoTexture, 2, sizeof(cl_int), &imHeight);
    errNum = clSetKernelArg(m_KernelCopyIntoTexture, 3, sizeof(cl_int), &m_OutputBuffer);
    errNum = clSetKernelArg(m_KernelCopyIntoTexture, 4, sizeof(cl_int), &m_GPUTab2D);
    errNum = clSetKernelArg(m_KernelCopyIntoTexture, 5, sizeof(cl_mem), &m_GPUTabNbPixelByColor);

    errNum = clEnqueueNDRangeKernel(commandQueue, m_KernelCopyIntoTexture, 2, NULL,  tex_globalWorkSize, 0, NULL, NULL, NULL);
    if (errNum != CL_SUCCESS)
    {
      std::cerr << "Error queuing kernel for execution." << std::endl;
    }
  }
  else
  {
    errNum = clSetKernelArg(m_KernelCopyIntoTextureBasic, 0, sizeof(cl_mem), &cl_tex_mem);
    errNum = clSetKernelArg(m_KernelCopyIntoTextureBasic, 1, sizeof(cl_int), &imWidth);
    errNum = clSetKernelArg(m_KernelCopyIntoTextureBasic, 2, sizeof(cl_int), &imHeight);
    errNum = clSetKernelArg(m_KernelCopyIntoTextureBasic, 3, sizeof(cl_int), &m_GPUTab2D);
    
    errNum = clEnqueueNDRangeKernel(commandQueue, m_KernelCopyIntoTextureBasic, 2, NULL, tex_globalWorkSize, 0, NULL, NULL, NULL);
    if (errNum != CL_SUCCESS)
    {
      std::cerr << "Error queuing kernel for execution." << std::endl;
    }
  }
  

  
  errNum = clEnqueueReleaseGLObjects(commandQueue, 1, &cl_tex_mem, 0, NULL, NULL);
  clFinish(commandQueue);

  return 0;
}


///
// Display the texture in the window
//
void QHoneycombWidget::displayTexture(int w, int h)
{
  glEnable(GL_TEXTURE_RECTANGLE_ARB);
  glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tex);
  glBegin(GL_QUADS);
  glTexCoord2f(0, 0);
  glVertex2f(0, 0);
  glTexCoord2f(0, h);
  glVertex2f(0, h);
  glTexCoord2f(w, h);
  glVertex2f(w, h);
  glTexCoord2f(w, 0);
  glVertex2f(w, 0);
  glEnd();
  glDisable(GL_TEXTURE_RECTANGLE_ARB);
}

void QHoneycombWidget::paintGL()
{
  glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  computeTexture();
  
  displayTexture(imWidth, imHeight);
  
  update();
  
  
}

void QHoneycombWidget::resizeGL(int w, int h)
{
  glViewport(0, 0, imHeight, imWidth);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0, imHeight, imWidth, 0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

/////////////////////////////////////////////////////

void QHoneycombWidget::Randomize()
{
  int NbAlea = 20000;
  for (int i = 0; i < NbAlea; i++)
  {
    int PlaceA1 = rand() % m_NbCell;
    int PlaceA2 = rand() % m_NbCell;
    int PlaceB1 = rand() % m_NbCell;
    int PlaceB2 = rand() % m_NbCell;


    int CoulA = GetArray2D(PlaceA1, PlaceA2);
    SetArray2D(PlaceA1, PlaceA2, GetArray2D(PlaceB1, PlaceB2));
    SetArray2D(PlaceB1, PlaceB2, CoulA);
  }

  int errNum = clEnqueueWriteBuffer(commandQueue, m_GPUTab2D, true, 0, m_NbCell * m_NbCell * sizeof(cl_uint), m_Tab2D.data(), 0, NULL, NULL);

  errNum = clFinish(commandQueue);
}

void QHoneycombWidget::InitTab2D()
{
  int larg = (int)(m_NbCell / sqrt((float)m_NbCoul));
  int NbCoulPerRow = (int)(sqrt((float)m_NbCoul));

  int index = 0;

  for (int i = 0; i < NbCoulPerRow; i++)
  {
    for (int j = 0; j < NbCoulPerRow; j++)
    {
      for (int ii = 0; ii < larg; ii++)
      for (int jj = 0; jj < larg; jj++)
        SetArray2D(ii + i*larg, jj + j*larg, index);
      index++;
    }
  }
}

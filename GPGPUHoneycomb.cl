__constant int RacCubiqueOfNbCol = 4;
__constant int DecalageCouleur = 3;





int GetArray2D(__global unsigned int * m_Tab2D, const int i, const int j)
{
  return m_Tab2D[i * m_NbCell + j] ;
}

void SetArray2D(__global unsigned int * m_Tab2D, const int i, const int j, const int iVal)
{
  m_Tab2D[i * m_NbCell + j] = iVal;
}







float PressureFromVolume(float densite) 
{
  const float pente = 2.;
  float pressure = 1. - pente*(1. - densite);
  if (pressure < 0.) pressure = 0.;
  return pressure;
}

int BoundCoordinate(int iCoord) 
{
  return
	(iCoord<0) ? iCoord+m_NbCell :
	(iCoord>=m_NbCell) ? iCoord-m_NbCell : iCoord;
}

int GetMeanOnBall( int iI, int iJ, __global unsigned int * m_TabNbPixelByColorInitial, __global unsigned int * m_TabNbPixelByColor, __global unsigned int * m_Tab2D)
{
	int NbCellsInBall = 0;
	int Coeff = 0;

    float m_TabNbCoulForMean[m_NbCoul];
	//float4 * m_TabNbCoulForMean4 = (float4*)m_TabNbCoulForMean;
	for(int k=0; k<m_NbCoul ; k++) m_TabNbCoulForMean[k] = 0.;
	
	//
	for (int i=-m_Rayon;i<=m_Rayon;i++)
	{
		for (int j=-m_Rayon;j<=m_Rayon;j++) 
		{
			if((i==0) && (j==0)) continue;
			int icour = BoundCoordinate(iI+i);
			int jcour = BoundCoordinate(iJ+j);
				
			bool IsInDisk = (i*i + j*j   < m_SquarredRadius);
				
			if (IsInDisk)
			{
				float x = convert_float(i);
				float y = convert_float(j);
				float weight = (m_SquarredRadius - x*x + y*y)*m_InvSquarredRadius;

				const float treshold = 1.;
				if( weight > treshold ) weight = treshold;

				int Color = GetArray2D(m_Tab2D, icour, jcour);
				float densite =  convert_float(m_TabNbPixelByColorInitial[Color]) /convert_float(m_TabNbPixelByColor[Color]);
				m_TabNbCoulForMean[Color] += weight*PressureFromVolume(densite);

			}

		}
	}
			//
	int NewCol = GetArray2D(m_Tab2D, iI, iJ); 
	for ( int jc= 0; jc < m_NbCoul; jc++)
	{
		if (m_TabNbCoulForMean[jc] >  m_TabNbCoulForMean[NewCol])
		{
			NewCol = jc;
		}
	}

    return NewCol;
}


__kernel void KelvinIteration(__global unsigned int * m_TabNbPixelByColorInitial, __global unsigned int * m_TabNbPixelByColor, __global unsigned int * m_Tab2D,
__global uint2 * m_OutputBuffer)
{

	unsigned int i = get_global_id(0);
	unsigned int j = get_global_id(1);

	

	int ColorBefore = GetArray2D(m_Tab2D, i, j);
	int ColorAfter = GetMeanOnBall ( i,j,  m_TabNbPixelByColorInitial, m_TabNbPixelByColor, m_Tab2D);

	m_OutputBuffer[i * m_NbCell + j].x = ColorAfter;
	m_OutputBuffer[i * m_NbCell + j].y = ColorBefore;
	
}


float4 GetColorFromUint(const unsigned int i)
{
  const float Inv = convert_float(RacCubiqueOfNbCol);
  float4 color =  { 
                      convert_float(1 + (i + DecalageCouleur) / (RacCubiqueOfNbCol*RacCubiqueOfNbCol)) / Inv,
					  convert_float(1 + (((i + DecalageCouleur) / (RacCubiqueOfNbCol)) % RacCubiqueOfNbCol)) / Inv,
					  convert_float(1 + (i + DecalageCouleur) % RacCubiqueOfNbCol) / Inv ,
					  1.f
			      };

  return color;
}




__kernel void copy_texture_kernel(__write_only image2d_t im, int w, int h, __global uint2 * InsideBuffer,  __global unsigned int * OutsideBuffer, volatile __global unsigned int * m_TabNbPixelByColor)
{
  unsigned int i = get_global_id(0);
  unsigned int j = get_global_id(1);

  unsigned int ColorAfter = InsideBuffer[i * w + j].x;
  unsigned int ColorBefore = InsideBuffer[i * m_NbCell + j].y;

	SetArray2D(OutsideBuffer, i , j , ColorAfter);
	atomic_dec( m_TabNbPixelByColor + ColorBefore);
	atomic_inc( m_TabNbPixelByColor + ColorAfter);
  
   
	int2 coord = { i,j };
	float4 color =  GetColorFromUint(ColorAfter);
	write_imagef( im, coord, color );
}

__kernel void copy_texture_kernel_basic(__write_only image2d_t im, int w, int h, __global unsigned int * InsideBuffer)
{
  unsigned int i = get_global_id(0);
  unsigned int j = get_global_id(1);

  unsigned int Color = InsideBuffer[i * w + j];
  
   
	int2 coord = { i,j };
	float4 color =  GetColorFromUint(Color);
	write_imagef( im, coord, color );
}

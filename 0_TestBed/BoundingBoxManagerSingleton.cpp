#include "BoundingBoxManagerSingleton.h"

//  BoundingBoxManagerSingleton
BoundingBoxManagerSingleton* BoundingBoxManagerSingleton::m_pInstance = nullptr;
void BoundingBoxManagerSingleton::Init(void)
{
	m_nBoxs = 0;
}
void BoundingBoxManagerSingleton::Release(void)
{
	//Clean the list of Boxs
	for(int n = 0; n < m_nBoxs; n++)
	{
		//Make sure to release the memory of the pointers
		if(m_lBox[n] != nullptr)
		{
			delete m_lBox[n];
			m_lBox[n] = nullptr;
		}
	}
	m_lBox.clear();
	m_lMatrix.clear();
	m_lColor.clear();
	m_nBoxs = 0;
}
BoundingBoxManagerSingleton* BoundingBoxManagerSingleton::GetInstance()
{
	if(m_pInstance == nullptr)
	{
		m_pInstance = new BoundingBoxManagerSingleton();
	}
	return m_pInstance;
}
void BoundingBoxManagerSingleton::ReleaseInstance()
{
	if(m_pInstance != nullptr)
	{
		delete m_pInstance;
		m_pInstance = nullptr;
	}
}
//The big 3
BoundingBoxManagerSingleton::BoundingBoxManagerSingleton(){Init();}
BoundingBoxManagerSingleton::BoundingBoxManagerSingleton(BoundingBoxManagerSingleton const& other){ }
BoundingBoxManagerSingleton& BoundingBoxManagerSingleton::operator=(BoundingBoxManagerSingleton const& other) { return *this; }
BoundingBoxManagerSingleton::~BoundingBoxManagerSingleton(){Release();};
//Accessors
int BoundingBoxManagerSingleton::GetBoxTotal(void){ return m_nBoxs; }

//--- Non Standard Singleton Methods
void BoundingBoxManagerSingleton::GenerateBoundingBox(matrix4 a_mModelToWorld, String a_sInstanceName)
{
	MeshManagerSingleton* pMeshMngr = MeshManagerSingleton::GetInstance();
	//Verify the instance is loaded
	if(pMeshMngr->IsInstanceCreated(a_sInstanceName))
	{//if it is check if the Box has already been created
		int nBox = IdentifyBox(a_sInstanceName);
		if(nBox == -1)
		{
			//Create a new bounding Box
			BoundingBoxClass* pBB = new BoundingBoxClass();
			//construct its information out of the instance name
			pBB->GenerateOrientedBoundingBox(a_sInstanceName);
			//Push the Box back into the list
			m_lBox.push_back(pBB);
			//Push a new matrix into the list
			m_lMatrix.push_back(matrix4(IDENTITY));
			//Specify the color the Box is going to have
			m_lColor.push_back(vector3(1.0f));
			//Increase the number of Boxes
			m_nBoxs++;
		}
		else //If the box has already been created you will need to check its global orientation
		{
			m_lBox[nBox]->GenerateAxisAlignedBoundingBox(a_mModelToWorld);
		}
		nBox = IdentifyBox(a_sInstanceName);
		m_lMatrix[nBox] = a_mModelToWorld;
	}
}

void BoundingBoxManagerSingleton::SetBoundingBoxSpace(matrix4 a_mModelToWorld, String a_sInstanceName)
{
	int nBox = IdentifyBox(a_sInstanceName);
	//If the Box was found
	if(nBox != -1)
	{
		//Set up the new matrix in the appropriate index
		m_lMatrix[nBox] = a_mModelToWorld;
	}
}

int BoundingBoxManagerSingleton::IdentifyBox(String a_sInstanceName)
{
	//Go one by one for all the Boxs in the list
	for(int nBox = 0; nBox < m_nBoxs; nBox++)
	{
		//If the current Box is the one we are looking for we return the index
		if(a_sInstanceName == m_lBox[nBox]->GetName())
			return nBox;
	}
	return -1;//couldn't find it return with no index
}

void BoundingBoxManagerSingleton::AddBoxToRenderList(String a_sInstanceName)
{
	//If I need to render all
	if(a_sInstanceName == "ALL")
	{
		for(int nBox = 0; nBox < m_nBoxs; nBox++)
		{
			m_lBox[nBox]->AddAABBToRenderList(m_lMatrix[nBox], m_lColor[nBox], true);
		}
	}
	else
	{
		int nBox = IdentifyBox(a_sInstanceName);
		if(nBox != -1)
		{
			m_lBox[nBox]->AddAABBToRenderList(m_lMatrix[nBox], m_lColor[nBox], true);
		}
	}
}

//Pass in index of the box 
bool BoundingBoxManagerSingleton::testSATOBB(int firstMat, int secondMat)
{
	//Calculating the center points for both boxes
	vector3 c1 = vector3(m_lMatrix[firstMat] * vector4(m_lBox[firstMat]->GetCentroid(), 1.0f));
	vector3 c2 = vector3(m_lMatrix[secondMat] * vector4(m_lBox[secondMat]->GetCentroid(), 1.0f));

	//Getting the x, y, and z values for the first matrix and storing them in another vector3 
	vector3 u1X = vector3(m_lMatrix[firstMat] * vector4(1, 0, 0, 0));
	vector3 u1Y = vector3(m_lMatrix[firstMat] * vector4(0, 1, 0, 0));
	vector3 u1Z = vector3(m_lMatrix[firstMat] * vector4(0, 0, 1, 0));
	vector3 u1[3] = {u1X, u1Y, u1Z};

	//Getting the x, y, and z values for the second matrix and storing them in another vector3 
	vector3 u2X = vector3(m_lMatrix[secondMat] * vector4(1, 0, 0, 0));
	vector3 u2Y = vector3(m_lMatrix[secondMat] * vector4(0, 1, 0, 0));
	vector3 u2Z = vector3(m_lMatrix[secondMat] * vector4(0, 0, 1, 0));
	vector3 u2[3] = {u2X, u2Y, u2Z};

	//Getting the distance from the edges of both boxes to their respective center points
	vector3 e1 = (m_lBox[firstMat]->GetSize())/2.0f;
	vector3 e2 = (m_lBox[secondMat]->GetSize())/2.0f;

	//Floats and matracies that will be used in the coming equasion
	float distance1;
	float distance2;
	matrix4 RMat;
	matrix4 AbsR;

	//Calculating a matrix for the two boxes' global corrdinates
	for(int i = 0; i < 3; i++)
	{
		for(int j = 0; j < 3; j++)
		{
			RMat[i][j] = glm::dot(u1[i], u2[j]);
		}
	}

	//Calculating a translation vector and bringing them into the global 
	vector3 t = c2 - c1;
	t = vector3(glm::dot(t, u1[0]), glm::dot(t, u1[1]), glm::dot(t, u1[2]));

	//Converting the above global matrix into the absolute value for that matrix, and storing it
	for(int i = 0; i < 3; i++)
	{
		for(int j = 0; j < 3; j++)
		{
			AbsR[i][j] = glm::abs(RMat[i][j]) + 0.000001;
		}
	}

	//TESTING THE INDIVIDUAL AXES. EVERY SINGLE AXIS WILL BE TESTED. SHOULD A SINGLE ONE PROVE TO BE NOT COLLIDING, THE FUNTION WILL RETURN A FLASE BOOLEAN AND CEASE.
	//OTHERWISE, THE FUNCTION WILL RETURN TRUE AT THE VERY END, SIGNIFYING THAT EVERY PLANE IS COLLIDING AND THUSLY, THAT THERE IS A COLLISION

	//L = A0, L = A1, L = A2
	for(int i = 0; i < 3; i++)
	{
		distance1 = e1[i];
		distance2 = e2[0] * AbsR[i][0] + e2[1] * AbsR[i][1] + e2[2] * AbsR[i][2];

		if(glm::abs(t[i]) > distance1 + distance2)
		{
			return false;
		}
	}

	//L = B0, L = B1, L = B2
	for(int i = 0; i < 3; i++)
	{
		distance1 = e1[0] * AbsR[i][0] + e1[1] * AbsR[i][1] + e1[2] * AbsR[i][2];
		distance2 = e2[i];	

		if(glm::abs(t[0] * RMat[0][i] + t[i] * RMat[1][i] + t[2] * RMat[2][i]) > distance1 + distance2)
		{
			return false;
		}
	}
	//L = AO X BO
	distance1 = e1[0] * AbsR[2][0] + e1[2] * AbsR[1][0];
	distance2 = e2[0] * AbsR[0][2] + e2[2] * AbsR[0][1];
	if(glm::abs(t[2]*RMat[1][0]-t[1]*RMat[2][0])>(distance1+distance2))
	{
		return false;
	}
	//L = A1 X B1
	distance1 = e1[1] * AbsR[2][1] + e1[2] * AbsR[1][1];
	distance2 = e2[0] * AbsR[0][2] + e2[2] * AbsR[0][0];
	if(glm::abs(t[2]*RMat[1][1]-t[1]*RMat[2][1])>(distance1+distance2))
	{
		return false;
	}
	//L = A0 X B2
	distance1 = e1[1] * AbsR[2][2] + e1[2] * AbsR[1][2];
	distance2 = e2[0] * AbsR[0][1] + e2[1] * AbsR[0][0];
	if(glm::abs(t[2]*RMat[1][2]-t[1]*RMat[2][2])>(distance1+distance2))
	{
		return false;
	}
	//L = A1 X B0
	distance1 = e1[0] * AbsR[2][0] + e1[2] * AbsR[0][0];
	distance2 = e2[1] * AbsR[1][2] + e2[2] * AbsR[1][1];
	if(glm::abs(t[0]*RMat[2][0]-t[2]*RMat[0][0])>(distance1+distance2))
	{
		return false;
	}
	//L = A1 X B1
	distance1 = e1[0] * AbsR[2][1] + e1[2] * AbsR[0][1];
	distance2 = e2[0] * AbsR[1][2] + e2[2] * AbsR[1][0];
	if(glm::abs(t[0]*RMat[2][1]-t[2]*RMat[0][1])>(distance1+distance2))
	{
		return false;
	}
	//L = A1 X B2
	distance1 = e1[0] * AbsR[2][2] + e1[2] * AbsR[0][2];
	distance2 = e2[0] * AbsR[1][1] + e2[1] * AbsR[1][0];
	if(glm::abs(t[0]*RMat[2][2]-t[2]*RMat[0][2])>(distance1+distance2))
	{
		return false;
	}
	//L = A2 X B0
	distance1 = e1[0] * AbsR[1][0] + e1[1] * AbsR[0][0];
	distance2 = e2[1] * AbsR[2][2] + e2[2] * AbsR[2][1];
	if(glm::abs(t[0]*RMat[2][1]-t[2]*RMat[0][0])>(distance1+distance2))
	{
		return false;
	}
	//L = A2 X B1
	distance1 = e1[0] * AbsR[1][1] + e1[1] * AbsR[0][1];
	distance2 = e2[0] * AbsR[2][2] + e2[2] * AbsR[2][0];
	if(glm::abs(t[1]*RMat[0][1]-t[0]*RMat[1][1])>(distance1+distance2))
	{
		return false;
	}
	//L = A2 X B2
	distance1 = e1[0] * AbsR[1][2] + e1[1] * AbsR[0][2];
	distance2 = e2[0] * AbsR[2][1] + e2[1] * AbsR[2][0];
	if(glm::abs(t[1]*RMat[0][2]-t[0]*RMat[1][2])>(distance1+distance2))
	{
		return false;
	}
	return true;
}

void BoundingBoxManagerSingleton::CalculateCollision(void)
{
	//Create a placeholder for all center points
	std::vector<vector3> lCentroid;
	//for all Boxs...
	for(int nBox = 0; nBox < m_nBoxs; nBox++)
	{
		//Make all the Boxs white
		m_lColor[nBox] = vector3(1.0f);
		//Place all the centroids of Boxs in global space
		lCentroid.push_back(static_cast<vector3>(m_lMatrix[nBox] * vector4(m_lBox[nBox]->GetCentroid(), 1.0f)));
	}

	//Now the actual check
	for(int i = 0; i < m_nBoxs - 1; i++)
	{
		for(int j = i + 1; j < m_nBoxs; j++)
		{
			//Replacing the previous AABB collision testing with the new SAT function.
			if(testSATOBB(i,j) == true)
			{
				m_lColor[i] = m_lColor[j] = MERED; //We make the Boxes red
			}			
		}
	}
}
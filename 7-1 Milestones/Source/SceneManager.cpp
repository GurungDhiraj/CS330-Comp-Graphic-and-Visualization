///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ,
	// Added offset value to make it easier to move around complex shapes
	glm::vec3 offset)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer and includes offset vector
	translation = glm::translate(positionXYZ + offset);

	// Vector multiplication works from right ot left which is why we scale, rotate, then translate
	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/***********************************************************
  *  LoadSceneTextures()
  *
  *  This method is used for preparing the 3D scene by loading
  *  the shapes, textures in memory to support the 3D scene
  *  rendering
  ***********************************************************/
void SceneManager::LoadSceneTextures()
{
	bool bReturn = false;

	// access the Textures folder for the table's texture
	bReturn = CreateGLTexture("./Textures/woodTableTexture.jpg", "table");
	// access the Textures folder for the backwall's texture
	bReturn = CreateGLTexture("./Textures/wallTexture.png", "wall");
	// access the Textures folder for the masking tape's texture
	bReturn = CreateGLTexture("./Textures/maskingTapeTexture.png", "maskingTape");
	// access the Textures folder for the small bottle cap's texture
	bReturn = CreateGLTexture("./Textures/smallBottleCapTexture.jpg", "smallBottleCap");
	// access the Textures folder for the perfume bottle's texture
	bReturn = CreateGLTexture("./Textures/perfumeBottleTexture.jpg", "perfumeBottleBase");
	// access the Textures folder for the perfume bottle's texture
	bReturn = CreateGLTexture("./Textures/perfumeBottleBaseText.png", "perfumeBottleBaseText");
	// access the Textures folder for the perfume bottle cap's texture
	bReturn = CreateGLTexture("./Textures/perfumeBottleCapTexture.png", "perfumeBottleCap");
	// access the Textures folder for the switch dock's texture
	bReturn = CreateGLTexture("./Textures/switchDockFrontText.png", "switchDockFrontText");
	// access the Textures folder for the switch dock's texture
	bReturn = CreateGLTexture("./Textures/switchDockTexture.png", "switchDock");


	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

// Method to configure various material and their light settings within the 3D scene
void SceneManager::DefineObjectMaterials()
{
	
	// Object Material for the plane
	OBJECT_MATERIAL tableMaterial;
	tableMaterial.ambientColor = glm::vec3(0.7f, 0.7f, 0.7f);
	tableMaterial.ambientStrength = 0.35f;
	tableMaterial.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f);
	tableMaterial.specularColor = glm::vec3(0.25f, 0.25f, 0.25f);
	tableMaterial.shininess = 0.5;
	tableMaterial.tag = "table";

	m_objectMaterials.push_back(tableMaterial);
	
	// Object Material for wall backwall
	OBJECT_MATERIAL backwallMaterial;
	backwallMaterial.ambientColor = glm::vec3(0.6f, 0.6f, 0.6f);
	backwallMaterial.ambientStrength = 0.35f;
	backwallMaterial.diffuseColor = glm::vec3(0.4f, 0.4f, 0.4f);
	backwallMaterial.specularColor = glm::vec3(0.0f, 0.0f, 0.0f);
	backwallMaterial.shininess = 0.0;
	backwallMaterial.tag = "backwall";

	m_objectMaterials.push_back(backwallMaterial);

	// Object Material for pill bottle
	OBJECT_MATERIAL glassMaterial;
	glassMaterial.ambientColor = glm::vec3(0.57, 0.70, 1.00); // sets ambient color to light blue
	glassMaterial.ambientStrength = 0.4f;
	glassMaterial.diffuseColor = glm::vec3(0.57, 0.70, 1.00); // sets diffuse color to light blue
	glassMaterial.specularColor = glm::vec3(0.6f, 0.6f, 0.6f);
	glassMaterial.shininess = 32.0;
	glassMaterial.tag = "glass";

	m_objectMaterials.push_back(glassMaterial);

	// Edit this for the Tape
	OBJECT_MATERIAL tapeMaterial;
	tapeMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f);
	tapeMaterial.ambientStrength = 0.2f;
	tapeMaterial.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f);
	tapeMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	tapeMaterial.shininess = 0.3;
	tapeMaterial.tag = "tape";

	m_objectMaterials.push_back(tapeMaterial);

	// Object Material for perfume bottle
	OBJECT_MATERIAL perfumeBottleMaterial;
	perfumeBottleMaterial.ambientColor = glm::vec3(0.0f, 0.0f, 0.0f);
	perfumeBottleMaterial.ambientStrength = 0.1f;
	perfumeBottleMaterial.diffuseColor = glm::vec3(0.1f, 0.1f, 0.1f);
	perfumeBottleMaterial.specularColor = glm::vec3(0.0f, 0.0f, 0.0f);
	perfumeBottleMaterial.shininess = 0.3;
	perfumeBottleMaterial.tag = "perfumeBottle";

	m_objectMaterials.push_back(perfumeBottleMaterial);

	// Object Material for Perfume Bottle Cap
	OBJECT_MATERIAL copperMaterial;
	copperMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	copperMaterial.ambientStrength = 0.6f;
	copperMaterial.diffuseColor = glm::vec3(0.94f, 0.47f, 0.37f);
	copperMaterial.specularColor = glm::vec3(0.5f, 0.5f, 0.5f);
	copperMaterial.shininess = 32.0;
	copperMaterial.tag = "copper";

	m_objectMaterials.push_back(copperMaterial);

	// Object Material for switch Dock
	OBJECT_MATERIAL switchDockMaterial;
	switchDockMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f);
	switchDockMaterial.ambientStrength = 0.1f;
	switchDockMaterial.diffuseColor = glm::vec3(0.1f, 0.1f, 0.1f);
	switchDockMaterial.specularColor = glm::vec3(0.4f, 0.2f, 0.2f);
	switchDockMaterial.shininess = 0.2;
	switchDockMaterial.tag = "dock";                      

	m_objectMaterials.push_back(switchDockMaterial);

}


// Method to add light sources and to adjust to each light source's attribute
void SceneManager::SetupSceneLights()
{
	/* this line of code tells the shaders to render the 3d scene with custom lighting 
	   - to use the default rendered lighting comment out the following line*/
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// sets main directional light to mimic a ceiling light placement
	m_pShaderManager->setVec3Value("directionalLight.direction", 0.0f, 12.0f, 10.0f); // sets position for directional light
	m_pShaderManager->setVec3Value("directionalLight.ambient", 0.1, 0.1, 0.1); //sets ambient light color
	m_pShaderManager->setVec3Value("directionalLight.diffuse", 0.82f, 0.93f, 0.96f);//sets diffuse light color to light blue
	m_pShaderManager->setVec3Value("directionalLight.specular", 0.1f, 0.1f, 0.1f); //sets specular light color
	m_pShaderManager->setBoolValue("directionalLight.bActive", true);

	// sets point light to add extra light in the scene
	m_pShaderManager->setVec3Value("pointLights[0].position", 0.0f, 3.0f, 8.0f); // sets position for pointLight
	m_pShaderManager->setVec3Value("pointLights[0].ambient", 0.05f, 0.05f, 0.05f); //sets ambient light color
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", 0.9f, 0.9f, 0.9f); //sets diffuse light color
	m_pShaderManager->setVec3Value("pointLights[0].specular", 0.1f, 0.1f, 0.1f); //sets specular light color
	m_pShaderManager->setBoolValue("pointLights[0].bActive", true);

}

/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// load the textures for the 3D scene
	LoadSceneTextures();
	
	// run function to define object material
	DefineObjectMaterials();

	// add and defile the light sources for the 3D scene
	SetupSceneLights();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadTorusMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
}
	

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	// Sets Default rotation values to 0
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;


	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	//PLANE GROUND
	scaleXYZ = glm::vec3(20.0f, 1.0f, 5.0f); // set the XYZ scale for the PLANE GROUND mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 5.0f); // set the XYZ position for the PLANE GROUND mesh

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	SetShaderTexture("table"); // uses the table texture for the PLANE GROUND
	SetShaderMaterial("table");
	m_basicMeshes->DrawPlaneMesh(); // draw the mesh with given transformation values
	/****************************************************************/

	//PLANE BACKWALL
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f); // set the XYZ scale for the mesh
	positionXYZ = glm::vec3(0.0f, 10.0f,0.0f); // set the XYZ position for the mesh

	// set the transformations into memory to be used on the drawn meshes
	// Applies a 90 deg rotation on x-axis to create a backwall
	SetTransformations(scaleXYZ, 90.0f, YrotationDegrees, ZrotationDegrees, positionXYZ);

	SetShaderTexture("wall"); // uses the wall texture for the PLANE BACKWALL
	SetShaderMaterial("backwall");
	m_basicMeshes->DrawPlaneMesh(); // draw the mesh with given transformation values
	//****************************************************************

	//MASKING TAPE
	scaleXYZ = glm::vec3(1.5f, 3.0f, 1.5f); // set the XYZ scale for the MASKING TAPE mesh
	positionXYZ = glm::vec3(-7.25f, 3.37f, 1.3f); // set the XYZ position for the MASKING TAPE mesh

	// set the transformations into memory to be used on the drawn meshes
	// sets x rotation to -20 deg to tilt tape against wall slightly
	SetTransformations(scaleXYZ, -20.0f, YrotationDegrees, ZrotationDegrees, positionXYZ);

	SetShaderTexture("maskingTape"); // uses the wall texture for the MASKING TAPE
	m_basicMeshes->DrawTorusMesh(); // draw the mesh with given transformation values
	//****************************************************************

	//offsett vector for SMALL BOTTLE to adjust position
	glm::vec3 smallBottleOffsetVector = glm::vec3(-3.25, 0.05f, 2.5f);

	//SMALL BOTTLE BASE
	// set the XYZ scale for the SMALL BOTTLE BASE mesh
	scaleXYZ = glm::vec3(0.25f, 1.0f, 0.25f);

	// set the XYZ position for the SMALL BOTTLE BASE mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ, smallBottleOffsetVector);

	SetShaderColor(0.57, 0.70, 1.00, 0.25); // SMALL BOTTLE BASE Color = Light Blue
	SetShaderMaterial("glass");
	m_basicMeshes->DrawCylinderMesh(); // draw the mesh with given transformation values
	//****************************************************************
	
	//SMALL BOTTLE NECK 
	// set the XYZ scale for the SMALL BOTTLE NECK  mesh
	scaleXYZ = glm::vec3(0.25f, 0.3f, 0.25f);

	// set the XYZ position for the SMALL BOTTLE NECK mesh
	positionXYZ = glm::vec3(0.0f, 1.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ, smallBottleOffsetVector);

	SetShaderColor(0.57, 0.70, 1.00, 0.5); // SMALL BOTTLE NECK Color = Light Blue
	SetShaderMaterial("glass");
	m_basicMeshes->DrawTaperedCylinderMesh(); // draw the mesh with given transformation values
	//****************************************************************
	//SetShaderMaterial("placeHolder");//*********
	//SMALL BOTTLE CAP 
	// set the XYZ scale for the SMALL BOTTLE CAP mesh
	scaleXYZ = glm::vec3(0.25f, 0.2f, 0.25f);

	// set the XYZ position for the SMALL BOTTLE CAP mesh
	positionXYZ = glm::vec3(0.0f, 1.2f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ, smallBottleOffsetVector);

	SetShaderTexture("smallBottleCap"); // uses the smallBottleCap texture for the SMALL BOTTLE CAP
	m_basicMeshes->DrawCylinderMesh(); // draw the mesh with given transformation values
	//****************************************************************
	
	//offsett vector for PERFUME BOTTLE to adjust position
	glm::vec3 perfumeBottleOffsetVector = glm::vec3(-1.5, 0.05f, 1.0f);

	//PERFUME BOTTLE BASE
	// set the XYZ scale for the PERFUME BOTTLE BASE mesh
	scaleXYZ = glm::vec3(2.0f, 3.0f, 0.75f);

	// set the XYZ position for the PERFUME BOTTLE BASE mesh
	positionXYZ = glm::vec3(0.0f, 1.5f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	// rotates perfume bottle by -15 degrees on the y-axis to be slightly angled 
	SetTransformations(scaleXYZ, XrotationDegrees, -15.0f, ZrotationDegrees, positionXYZ, perfumeBottleOffsetVector);

	SetShaderTexture("perfumeBottleBaseText"); // uses the perfumeBottleBaseText texture for the PERFUME BOTTLE BASE
	m_basicMeshes->DrawBoxSideMesh(ShapeMeshes::BoxSide::front); // Draw only the front side of box so that the text is only on the front
	
	SetShaderTexture("perfumeBottleBase"); // uses the perfumeBottleBase texture for the PERFUME BOTTLE BASE
	SetShaderMaterial("perfumeBottle");
	m_basicMeshes->DrawBoxMesh(); // draw the mesh with given transformation values
	//****************************************************************

	//PERFUME BOTTLE CAP
	// set the XYZ scale for the PERFUME BOTTLE CAP mesh
	scaleXYZ = glm::vec3(0.4f, 0.3f, 0.3f);

	// set the XYZ position for the PERFUME BOTTLE CAP mesh
	positionXYZ = glm::vec3(0.0f, 3.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ, perfumeBottleOffsetVector);

	SetShaderTexture("perfumeBottleCap"); // uses the perfumeBottleCap texture for the PERFUME BOTTLE CAP
	SetShaderMaterial("copper");
	m_basicMeshes->DrawCylinderMesh(); // draw the mesh with given transformation values
	//****************************************************************
	
	//offsett vector for SWITCH DOCK to adjust position
	glm::vec3 switchDockOffsetVector = glm::vec3(2.5, 0.475f, 1.0f);

	//SWITCH DOCK FRONT
	// set the XYZ scale for the SWITCH DOCK FRONT mesh
	scaleXYZ = glm::vec3(5.0f, 3.75f, 0.15f);

	// set the XYZ position for the SWITCH DOCK FRONT mesh
	positionXYZ = glm::vec3(0.0f, 1.4f, 1.0f);

	// set the transformations into memory to be used on the drawn meshes
	// rotates Dock -20deg on y-axis to angle it in same direction as perfume bottle
	SetTransformations(scaleXYZ, XrotationDegrees, -20.0f, ZrotationDegrees, positionXYZ, switchDockOffsetVector);

	SetShaderTexture("switchDockFrontText");// uses the switchDockText texture for the front of the SWITCH DOCK
	m_basicMeshes->DrawBoxSideMesh(ShapeMeshes::BoxSide::front); // draw the mesh with given transformation values
	SetShaderTexture("switchDock");// uses the switchDockTexture texture for the rest of the SWITCH DOCK FRONT
	SetShaderMaterial("dock");
	m_basicMeshes->DrawBoxMesh(); // draw the mesh with given transformation values
	//****************************************************************

	//SWITCH DOCK MIDDLE
	// set the XYZ scale for the SWITCH DOCK MIDDLE mesh
	scaleXYZ = glm::vec3(5.0f, 1.0f, 0.4f);

	// set the XYZ position for the SWITCH DOCK MIDDLE mesh
	positionXYZ = glm::vec3(0.095f, 0.03f, 0.75f);

	// set the transformations into memory to be used on the drawn meshes
	// rotates Dock -20deg on y-axis to angle it in same direction as perfume bottle
	SetTransformations(scaleXYZ, XrotationDegrees, -20.0f, ZrotationDegrees, positionXYZ, switchDockOffsetVector);

	SetShaderTexture("switchDock");// uses the switchDockTexture texture for SWITCH DOCK MIDDLE
	SetShaderMaterial("dock");
	m_basicMeshes->DrawBoxMesh(); // draw the mesh with given transformation values

	//SWITCH DOCK BACK
	// set the XYZ scale for the SWITCH DOCK BACK mesh
	scaleXYZ = glm::vec3(5.0f, 3.75f, 0.75f);

	// set the XYZ position for the SWITCH DOCK BACK mesh
	positionXYZ = glm::vec3(0.25f, 1.4f, 0.3f);

	// set the transformations into memory to be used on the drawn meshes
	// rotates Dock -20deg on y-axis to angle it in same direction as perfume bottle
	SetTransformations(scaleXYZ, XrotationDegrees, -20.0f, ZrotationDegrees, positionXYZ, switchDockOffsetVector);

	SetShaderTexture("switchDock");// uses the switchDockTexture texture for SWITCH DOCK BACK
	SetShaderMaterial("dock");
	m_basicMeshes->DrawBoxMesh(); // draw the mesh with given transformation values
	
}

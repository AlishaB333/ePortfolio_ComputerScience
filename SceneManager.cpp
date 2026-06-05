///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//  MODIFIED BY: Alisha Brayboy - SNHU Student
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>
// for time query to animate sun
#include <GLFW/glfw3.h>
#include <cmath>

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

	// initialize the texture collection
	for (int i = 0; i < 16; i++)
	{
		m_textureIDs[i].tag = "/0";
		m_textureIDs[i].ID = -1;
	}
	m_loadedTextures = 0;
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	// clear the allocated memory
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
	// destroy the created OpenGL textures
	DestroyGLTextures();
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
	if (m_loadedTextures >= 16)
	{
		std::cout << "Texture capacity reached (16). Cannot load: " << filename << std::endl;
		return false;
	}
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
			stbi_image_free(image);
			glBindTexture(GL_TEXTURE_2D, 0);
			// delete generated texture to avoid leak
			if (textureID != 0) glDeleteTextures(1, &textureID);
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

	std::cout << "Could not load image: " << filename << " - " << stbi_failure_reason() << std::endl;

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
		GLuint id = static_cast<GLuint>(m_textureIDs[i].ID);
		if (id != 0 && id != static_cast<GLuint>(-1))
		{
			glDeleteTextures(1, &id);
			m_textureIDs[i].ID = 0;
			m_textureIDs[i].tag = "/0";
		}
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

	return(bFound);
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
	glm::vec3 positionXYZ)
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
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

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
		int textureSlot = FindTextureSlot(textureTag);
		if (textureSlot < 0)
		{
			// texture not found - disable texture use
			m_pShaderManager->setIntValue(g_UseTextureName, false);
			// optionally bind texture unit 0 to avoid undefined sampler value
			m_pShaderManager->setSampler2DValue(g_TextureValueName, 0);
		}
		else
		{
			m_pShaderManager->setIntValue(g_UseTextureName, true);
			m_pShaderManager->setSampler2DValue(g_TextureValueName, textureSlot);
		}
	}
}

void SceneManager::SetShaderTextures(std::string textureTag1, std::string textureTag2)
{
	if (m_pShaderManager != NULL)
	{
		int textureID1 = FindTextureSlot(textureTag1);
		int textureID2 = FindTextureSlot(textureTag2);

		bool any = (textureID1 >= 0) || (textureID2 >= 0);
		m_pShaderManager->setIntValue(g_UseTextureName, any);

		if (textureID1 >= 0)
			m_pShaderManager->setSampler2DValue("texture1", textureID1);
		else
			m_pShaderManager->setSampler2DValue("texture1", 0);

		if (textureID2 >= 0)
			m_pShaderManager->setSampler2DValue("texture2", textureID2);
		else
			m_pShaderManager->setSampler2DValue("texture2", 0);
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
  *  LoadSceneTextures()
  *
  *  This method is used for preparing the 3D scene by loading
  *  the shapes, textures in memory to support the 3D scene
  *  rendering
  ***********************************************************/
void SceneManager::LoadSceneTextures()
{

	bool bReturn = false;

	// Use forward-slashes or raw-string literals to avoid escape-sequence issues
	bReturn = CreateGLTexture(
		"Debug/dirt_seamless.jpg",
		"dirt");
	bReturn = CreateGLTexture(
		"Debug/gravel_seamless.jpg",
		"gravel");
	bReturn = CreateGLTexture(
		"Debug/wood_planks.jpg",
		"planks");
	bReturn = CreateGLTexture(
		"Debug/sandbox_wood.jpg",
		"side");
	bReturn = CreateGLTexture(
		"Debug/sink.jpg",
		"sink");
	bReturn = CreateGLTexture(
		"Debug/fence.jpg",
		"fence");
	bReturn = CreateGLTexture(
		"Debug/blackboard.jpg",
		"chalk");
	bReturn = CreateGLTexture(
		"Debug/truck_plastic.jpg",
		"plastic");
	bReturn = CreateGLTexture(
		"Debug/truck_cab.jpg",
		"cab");

	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();
}

/***********************************************************
  *  DefineObjectMaterials()
  *
  *  This method is used for configuring the various material
  *  settings for all of the objects within the 3D scene.
  ***********************************************************/
void SceneManager::DefineObjectMaterials()
{

	// Define material for all objects
	OBJECT_MATERIAL woodMaterial;
	woodMaterial.ambientColor = glm::vec3(0.4f, 0.3f, 0.1f); // Light gray for ambient color
	woodMaterial.ambientStrength = 0.2f; // Light ambient strength
	woodMaterial.diffuseColor = glm::vec3(0.3, 0.2f, 0.1f); // Soft base color
	woodMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f); //Lower specular color for a more subtle highlight
	woodMaterial.shininess = 0.3f; //Low shinisness for a more matte finish
	woodMaterial.tag = "wood";

	// Pass these materials to the shader
	m_objectMaterials.push_back(woodMaterial);

	// Define material for all objects
	OBJECT_MATERIAL stoneMaterial;
	stoneMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f); // Light gray for ambient color
	stoneMaterial.ambientStrength = 0.2f; // Light ambient strength
	stoneMaterial.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f); // Medium base color
	stoneMaterial.specularColor = glm::vec3(0.4f, 0.4f, 0.4f); //higherer specular color for a defined highlight
	stoneMaterial.shininess = 0.5f; //Medium shinisness for a more matte finish
	stoneMaterial.tag = "stone";

	// Pass these materials to the shader
	m_objectMaterials.push_back(stoneMaterial);

	// Define material for all objects
	OBJECT_MATERIAL metalMaterial;
	metalMaterial.ambientColor = glm::vec3(0.3f, 0.3f, 0.3f); // Light gray for ambient color
	metalMaterial.ambientStrength = 0.4f; // Medium ambient strength
	metalMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.2f); // Medium base color
	metalMaterial.specularColor = glm::vec3(0.6f, 0.5f, 0.4f); //higherer specular color for a defined highlight
	metalMaterial.shininess = 22.0f; //High shinisness for a more glossy finish
	metalMaterial.tag = "metal";

	// Pass these materials to the shader
	m_objectMaterials.push_back(metalMaterial);

	// Define material for all objects
	OBJECT_MATERIAL plasticMaterial;
	plasticMaterial.ambientColor = glm::vec3(0.3f, 0.3f, 0.3f); // Light gray for ambient color
	plasticMaterial.ambientStrength = 0.4f; // Medium ambient strength
	plasticMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.2f); // Medium base color
	plasticMaterial.specularColor = glm::vec3(0.6f, 0.5f, 0.4f); //higherer specular color for a defined highlight
	plasticMaterial.shininess = 15.0f; //Higher shininess for a glossy finish
	plasticMaterial.tag = "plastic";

	// Pass these materials to the shader
	m_objectMaterials.push_back(plasticMaterial);

	// Define material for all objects
	OBJECT_MATERIAL dirtMaterial;
	dirtMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.3f); // Light gray for ambient color
	dirtMaterial.ambientStrength = 0.3f; // Medium ambient strength
	dirtMaterial.diffuseColor = glm::vec3(0.4f, 0.4f, 0.5f); // Medium base color
	dirtMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.4f); //higherer specular color for a defined highlight
	dirtMaterial.shininess = 0.5f; //Low shininess for subtle finish
	dirtMaterial.tag = "dirt";

	// Pass these materials to the shader
	m_objectMaterials.push_back(dirtMaterial);
}

/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene.  There are up to 4 light sources.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	if (m_pShaderManager == NULL) return;

	m_pShaderManager->setBoolValue("bUseLighting", true);

	// set the light source 1 values in the shader
	m_pShaderManager->setIntValue("lightSources[0].type", 0); // directional light
	m_pShaderManager->setVec3Value("lightSources[0].direction", -0.2f, -1.0f, -0.3f);
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 1.0f, 1.0f, 0.6f); //Light yellow for ambient color
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 0.5f, 0.5f, 0.5f);
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 48.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.5f);
	
	// set the light source 2 values in the shader
	m_pShaderManager->setIntValue("lightSources[1].type", 1); // point light
	m_pShaderManager->setVec3Value("lightSources[1].position", 2.0f, 4.0f, -2.0f);
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 1.0f, 1.0f, 0.6f); //Light yellow for ambient color
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.8f, 0.8f, 0.8f);
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 48.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.5f);
}

/***********************************************************
 *  UpdateAnimation()
 *
 *  This method is called to update the animation parameters
 *  for the 3D scene.  It is typically called once per frame.
 ***********************************************************/

void SceneManager::UpdateAnimation(float deltaTime)
{
	// parameters you can tune
	const float truckSpeed = 1.2f;        // world units per second
	const float wheelRadius = 0.1f;       // match wheel scale used when drawing
	const float twoPi = 6.28318530718f;

	// advance truck position
	float distance = truckSpeed * deltaTime;
	m_truckOffset += distance;

	// wrap or clamp offset if desired (example wraps every 30 units)
	const float wrapRange = 30.0f;
	if (m_truckOffset > wrapRange) m_truckOffset -= wrapRange;

	// convert traveled distance to wheel rotation degrees
	float circumference = twoPi * wheelRadius;
	if (circumference > 0.0f)
	{
		float revs = distance / circumference;
		m_wheelAngle += revs * 360.0f;
		// keep angle in reasonable range
		m_wheelAngle = fmodf(m_wheelAngle, 360.0f);
	}
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
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
	m_basicMeshes->LoadTorusMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadPrismMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	/*
	TOY TRUCK ANIMATION - ENHANCEMENT
	*/
	// compute time/angle (existing)
	float currentTime = (float)glfwGetTime();
	// ensure first-frame initialization
	if (m_lastTime <= 0.0f) m_lastTime = currentTime;
	float deltaTime = currentTime - m_lastTime;
	m_lastTime = currentTime;

	// update animation state
	UpdateAnimation(deltaTime);
	
	/*
	SUNLIGHT ANIMATION - ENHANCEMENT
	*/
	// compute time/angle for Sunlight animation
	float time = currentTime;
	float speed = 0.2f; // radians/sec
	float angle = time * speed;

	// Sun moves across the sky in X-Y plane, with a small upward bias
	glm::vec3 sunDir = glm::normalize(glm::vec3(cos(angle), sin(angle), 0.25f)); // points from surface to sun

	// compute elevation factor (1 when overhead, -1 below horizon)
	float elevation = glm::dot(sunDir, glm::vec3(0.0f, 0.0f, 1.0f));

	// determine color and intensity from elevation
	glm::vec3 dayColor(1.0f, 0.95f, 0.9f);
	glm::vec3 sunsetColor(1.0f, 0.5f, 0.2f);
	glm::vec3 nightColor(0.02f, 0.04f, 0.08f);

	glm::vec3 sunColor;
	float intensity;
	if (elevation > 0.0f) {
		// daytime: blend from warm at low elevation to white at high elevation
		float t = glm::clamp(elevation, 0.0f, 1.0f);
		sunColor = glm::mix(sunsetColor, dayColor, pow(t, 0.6f));
		intensity = glm::mix(0.2f, 1.2f, t); // stronger when higher
	} else {
		// below horizon: faint moon-like ambient
		sunColor = nightColor;
		intensity = 0.02f;
	}

	// send to shader (assume shader program bound and uniform locations found)
	if (m_pShaderManager != NULL)
	{
		m_pShaderManager->setVec3Value("sunDirection", sunDir);
		m_pShaderManager->setVec3Value("sunColor", sunColor * intensity);
		m_pShaderManager->setFloatValue("sunElevation", elevation);
	}

	//PLANE BASE
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("dirt");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("dirt");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();


	//SANDBOX
	//Create GRAVEL BASE
	// set the XYZ scale of gravel
	scaleXYZ = glm::vec3(12.0f, 0.5f, 10.0f);

	// set the XYZ rotation of gravel
	XrotationDegrees = 0.0f;
	YrotationDegrees = -10.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of gravel
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 0, 0, 1);
	SetShaderTexture("gravel");
	SetTextureUVScale(2.0f, 1.0f);
	SetShaderMaterial("stone");

	m_basicMeshes->DrawBoxMesh();
	
	
	//Create NEGATIVE SPACE on one corner of box gravel
	// set the XYZ scale of negative space
	scaleXYZ = glm::vec3(7.0f, 1.0f, 7.0f);

	// set the XYZ rotation of negative space
	XrotationDegrees = 0.0f;
	YrotationDegrees = -92.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of negative space
	positionXYZ = glm::vec3(3.0f, 0.0f, 5.3f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("dirt");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("dirt");

	m_basicMeshes->DrawPrismMesh();

	
	
	//Create SANDBOX SIDE 1 (Top Side)
	// set the XYZ scale of sandbox side 1
	scaleXYZ = glm::vec3(12.2f, 1.5f, 0.2f);

	// set the XYZ rotation of sandbox side 1
	XrotationDegrees = 0.0f;
	YrotationDegrees = -10.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of sandbox side 1
	positionXYZ = glm::vec3(0.8f, 0.0f, -5.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 0, 1, 1);
	SetShaderTexture("side");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("wood");

	m_basicMeshes->DrawBoxMesh();

	
	
	//Create SANDBOX SIDE 2 (Left Side)
	// set the XYZ scale of sandbox side 2
	scaleXYZ = glm::vec3(10.0f, 1.5f, 0.2f);

	// set the XYZ rotation of sandbox side 2
	XrotationDegrees = 0.0f;
	YrotationDegrees = 80.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of sandbox side 2
	positionXYZ = glm::vec3(-6.0f, 0.0f, -1.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 0, 1, 1);
	SetShaderTexture("side");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("wood");

	m_basicMeshes->DrawBoxMesh();

	
	//Create SANDBOX SIDE 3 (Bottom Side)
	// set the XYZ scale of sandbox side 3
	scaleXYZ = glm::vec3(6.8f, 1.5f, 0.2f);

	// set the XYZ rotation of sandbox side 3
	XrotationDegrees = 0.0f;
	YrotationDegrees = -10.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of sandbox side 3
	positionXYZ = glm::vec3(-3.6f, 0.0f, 4.5f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	
	SetShaderColor(1, 0, 1, 1);
	SetShaderTexture("side");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("wood");

	m_basicMeshes->DrawBoxMesh();


	//Create SANDBOX SIDE 4 (Right Side)
	// set the XYZ scale of sandbox side 4
	scaleXYZ = glm::vec3(6.5f, 1.3f, 0.2f);

	// set the XYZ rotation of sandbox side 4
	XrotationDegrees = 0.0f;
	YrotationDegrees = 80.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of sandbox side 4
	positionXYZ = glm::vec3(6.2f, 0.0f, -1.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 0, 1, 1);
	SetShaderTexture("side");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("wood");

	m_basicMeshes->DrawBoxMesh();

	//Create SANDBOX SIDE 5 (Diagonal Side)
	// set the XYZ scale of sandbox side 5
	scaleXYZ = glm::vec3(6.8f, 1.5f, 0.2f);

	// set the XYZ rotation of sandbox side 5
	XrotationDegrees = 0.0f;
	YrotationDegrees = 27.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of sandbox side 5
	positionXYZ = glm::vec3(2.8f, 0.0f, 3.5f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 0, 1, 1);
	SetShaderTexture("side");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("wood");

	m_basicMeshes->DrawBoxMesh();

	
	
	//ROCK KITCHEN
	//Create Cupboard
	// set the XYZ scale of cupboard
	scaleXYZ = glm::vec3(2.0f, 2.0f, 2.0f);

	// set the XYZ rotation of cupboard
	XrotationDegrees = 0.0f;
	YrotationDegrees = -20.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of cupboard
	positionXYZ = glm::vec3(-4.5f, 2.0f, 0.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 1, 0, 1);
	SetShaderTexture("planks");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("wood");

	m_basicMeshes->DrawBoxMesh();

	//Create Cupboard Door Handle
	scaleXYZ = glm::vec3(0.08f, 0.5f, 0.08f);

	// set the XYZ rotation of Handle
	XrotationDegrees = 0.0f;
	YrotationDegrees = -20.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of Handle
	positionXYZ = glm::vec3(-3.4f, 2.0f, 0.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(0, 0, 0, 0);
	SetShaderTexture("chalk");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("stone");

	m_basicMeshes->DrawBoxMesh();

	//Create 4 legs of Cupboard
	//Leg 1 (Front left leg)
	// set the XYZ scale of Leg 1
	scaleXYZ = glm::vec3(0.3f, 2.0f, 0.3f);

	// set the XYZ rotation of Leg 1
	XrotationDegrees = 0.0f;
	YrotationDegrees = -20.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of Leg 1
	positionXYZ = glm::vec3(-4.0f, 1.0f, 1.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 1, 0, 1);
	SetShaderTexture("planks");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("wood");

	m_basicMeshes->DrawBoxMesh();

	//Leg 2 (Back left leg)
	// set the XYZ scale of Leg 2
	scaleXYZ = glm::vec3(0.3f, 2.0f, 0.3f);

	// set the XYZ rotation of Leg 2
	XrotationDegrees = 0.0f;
	YrotationDegrees = -20.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of Leg 2
	positionXYZ = glm::vec3(-5.5f, 1.0f, 0.5f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 1, 0, 1);
	SetShaderTexture("planks");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("wood");
	
	m_basicMeshes->DrawBoxMesh();

	//Leg 3 (Front right leg)
	// set the XYZ scale of Leg 3
	scaleXYZ = glm::vec3(0.3f, 2.0f, 0.3f);

	// set the XYZ rotation of Leg 3
	XrotationDegrees = 0.0f;
	YrotationDegrees = -20.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of Leg 3
	positionXYZ = glm::vec3(-3.5f, 1.0f, -0.5f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 1, 0, 1);
	SetShaderTexture("planks");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("wood");
	
	m_basicMeshes->DrawBoxMesh();

	//Leg 4 (Back right leg)
	// set the XYZ scale of Leg 4
	scaleXYZ = glm::vec3(0.3f, 2.0f, 0.3f);

	// set the XYZ rotation of Leg 4
	XrotationDegrees = 0.0f;
	YrotationDegrees = -20.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of Leg 4
	positionXYZ = glm::vec3(-5.0f, 1.0f, -1.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 1, 0, 1);
	SetShaderTexture("planks");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("wood");
	
	m_basicMeshes->DrawBoxMesh();
	

	//Create 2 Long Legs
	//Leg 1 (Long right leg)
	// set the XYZ scale of Leg 1
	scaleXYZ = glm::vec3(0.3f, 3.0f, 0.3f);

	// set the XYZ rotation of Leg 1
	XrotationDegrees = 0.0f;
	YrotationDegrees = -20.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of Leg 1
	positionXYZ = glm::vec3(-2.5f, 1.5f, -3.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 1, 0, 1);
	SetShaderTexture("planks");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("wood");
	
	m_basicMeshes->DrawBoxMesh();


	//Leg 2 (Long left leg)
	// set the XYZ scale of Leg 2
	scaleXYZ = glm::vec3(0.3f, 3.0f, 0.3f);

	// set the XYZ rotation of Leg 2
	XrotationDegrees = 0.0f;
	YrotationDegrees = -20.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of Leg 2
	positionXYZ = glm::vec3(-4.0f, 1.5f, -3.5f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 1, 0, 1);
	SetShaderTexture("planks");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("wood");
	
	m_basicMeshes->DrawBoxMesh();


	//Create tabletop
	// set the XYZ scale of tabletop
	scaleXYZ = glm::vec3(2.0f, 0.2f, 4.8f);

	// set the XYZ rotation of tabletop
	XrotationDegrees = 0.0f;
	YrotationDegrees = -20.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of tabletop
	positionXYZ = glm::vec3(-4.0f, 3.0f, -1.2f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 1, 0, 1);
	SetShaderTexture("planks");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("wood");

	m_basicMeshes->DrawBoxMesh();

	//Create Sink
	// set the XYZ scale of sinktub
	scaleXYZ = glm::vec3(1.0f, 0.3f, 1.5f);

	// set the XYZ rotation of sinktub
	XrotationDegrees = 0.0f;
	YrotationDegrees = -20.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of sinktub
	positionXYZ = glm::vec3(-3.5f, 3.0f, -1.5f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 1, 0, 1);
	SetShaderTexture("sink");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("metal");

	m_basicMeshes->DrawBoxMesh();

	//Create Faucet
	
	//Faucett Stem 1
	// set the XYZ scale of Stem 1
	scaleXYZ = glm::vec3(0.05f, 0.8f, 0.05f);

	// set the XYZ rotation of Stem 1
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of Stem 1
	positionXYZ = glm::vec3(-4.0f, 3.0f, -1.8f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 1, 0, 1);
	SetShaderTexture("sink");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("metal");

	m_basicMeshes->DrawCylinderMesh();

	//Faucett Stem 1
	// set the XYZ scale of Stem 2
	scaleXYZ = glm::vec3(0.05f, 0.5f, 0.05f);

	// set the XYZ rotation of Stem 2
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -55.0f;

	// set the XYZ position of Stem 2
	positionXYZ = glm::vec3(-4.0f, 3.6f, -1.8f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 1, 0, 1);
	SetShaderTexture("sink");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("metal");

	m_basicMeshes->DrawCylinderMesh();

	//Sphere for faucestt bulb
	// set the XYZ scale of bulb
	scaleXYZ = glm::vec3(0.08f, 0.08f, 0.08f);

	// set the XYZ rotation of bulb
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of bulb
	positionXYZ = glm::vec3(-3.6f, 3.85f, -1.8f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 1, 0, 1);
	SetShaderTexture("sink");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("metal");

	m_basicMeshes->DrawSphereMesh();

	//Create 3 backboard stands

	//Backboard stand 1 (Right stand)
	// set the XYZ scale of BB Stand 1
	scaleXYZ = glm::vec3(0.3f, 3.0f, 0.3f);

	// set the XYZ rotation of BB Stand 1
	XrotationDegrees = 0.0f;
	YrotationDegrees = -20.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of BB Stand 1
	positionXYZ = glm::vec3(-4.1f, 4.5f, -3.5f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 1, 0, 1);
	SetShaderTexture("planks");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("wood");

	m_basicMeshes->DrawBoxMesh();

	//Backboard stand 2 (Middle stand)
	scaleXYZ = glm::vec3(0.3f, 3.0f, 0.3f);

	// set the XYZ rotation of BB Stand 2
	XrotationDegrees = 0.0f;
	YrotationDegrees = -20.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of BB Stand 2
	positionXYZ = glm::vec3(-5.0f, 4.5f, -1.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 1, 0, 1);
	SetShaderTexture("planks");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("wood");

	m_basicMeshes->DrawBoxMesh();

	//Backboard stand 3 (Left stand)
	scaleXYZ = glm::vec3(0.3f, 3.0f, 0.3f);

	// set the XYZ rotation of BB Stand 3
	XrotationDegrees = 0.0f;
	YrotationDegrees = -20.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of BB Stand 3
	positionXYZ = glm::vec3(-5.5f, 4.5f, 0.5f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 1, 0, 1);
	SetShaderTexture("planks");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("wood");

	m_basicMeshes->DrawBoxMesh();


	//Create backboard sign
	scaleXYZ = glm::vec3(0.5f, 1.0f, 2.5f);

	// set the XYZ rotation of BB Sign
	XrotationDegrees = 0.0f;
	YrotationDegrees = -20.0f;
	ZrotationDegrees = -90.0f;

	// set the XYZ position of BB Sign
	positionXYZ = glm::vec3(-4.6f, 5.5f, -1.5f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(0, 0, 0, 0);
	SetShaderTexture("chalk");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("stone");

	m_basicMeshes->DrawPlaneMesh();



	//TOY CEMENT TRUCK - ENHANCED

	//Create 4 Tires
	//Tire 1 (Back Passenger side tire)
	// set the XYZ scale of Back PS Tire
	scaleXYZ = glm::vec3(0.1f, 0.1f, 0.1f);

	// wheel spin should rotate the wheel around its local X axis (tweak axis if visually wrong)
	XrotationDegrees = m_wheelAngle;      // spin
	YrotationDegrees = 15.0f;             // existing tilt
	ZrotationDegrees = 0.0f;

	// base position as before, then apply truck offset along X
	positionXYZ = glm::vec3(2.5f, 0.4f, 0.65f);
	positionXYZ += glm::vec3(m_truckOffset, 0.0f, 0.0f); // move with truck

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(0, 0, 0, 1);
	SetShaderTexture("chalk");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("stone");

	m_basicMeshes->DrawTorusMesh();

	//Tire 1 Hubcap
	// set the XYZ scale of HC 1
	scaleXYZ = glm::vec3(0.08f, 0.08f, 0.08f);

	// set the XYZ rotation of HC 1
	// hubcap spins with the wheel as well
	XrotationDegrees = m_wheelAngle;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of HC 1 then move with wheel
	positionXYZ = glm::vec3(2.5f, 0.4f, 0.65f);
	positionXYZ += glm::vec3(m_truckOffset, 0.0f, 0.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(0, 0, 0, 1);
	SetShaderTexture("plastic");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("plastic");

	m_basicMeshes->DrawSphereMesh();


	//Tire 2 (Front Passenger side tire)
	// set the XYZ scale of Front PS Tire
	scaleXYZ = glm::vec3(0.1f, 0.1f, 0.1f);

	// wheel spin should rotate the wheel around its local X axis (tweak axis if visually wrong)
	XrotationDegrees = m_wheelAngle;      // spin
	YrotationDegrees = 15.0f;			// existing tilt
	ZrotationDegrees = 0.0f;

	//base position of Front PS Tire, then apply truck offset along X
	positionXYZ = glm::vec3(1.5f, 0.4f, 0.5f);
	positionXYZ += glm::vec3(m_truckOffset, 0.0f, 0.0f); // move with truck

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(0, 0, 0, 1);
	SetShaderTexture("chalk");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("stone");

	m_basicMeshes->DrawTorusMesh();

	//Tire 2 Hubcap
	// set the XYZ scale of HC 2
	scaleXYZ = glm::vec3(0.08f, 0.08f, 0.08f);

	// hubcap spins with the wheel as well
	XrotationDegrees = m_wheelAngle;
	YrotationDegrees = 0.0f;			
	ZrotationDegrees = 0.0f;

	// set the XYZ position of HC 2
	positionXYZ = glm::vec3(1.5f, 0.4f, 0.5f);
	positionXYZ += glm::vec3(m_truckOffset, 0.0f, 0.0f); // move with truck

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(0, 0, 0, 1);
	SetShaderTexture("plastic");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("plastic");

	m_basicMeshes->DrawSphereMesh();


	//Tire 3 (Back Driver side tire)
	// set the XYZ scale of Back DS Tire
	scaleXYZ = glm::vec3(0.1f, 0.1f, 0.1f);

	// wheel spin should rotate the wheel around its local X axis (tweak axis if visually wrong)
	XrotationDegrees = m_wheelAngle;      // spin
	YrotationDegrees = 12.0f;			//existing tilt
	ZrotationDegrees = 0.0f;

	// set the XYZ position of Back DS Tire
	positionXYZ = glm::vec3(2.5f, 0.4f, 1.15f);
	positionXYZ += glm::vec3(m_truckOffset, 0.0f, 0.0f); // move with truck

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(0, 0, 0, 1);
	SetShaderTexture("chalk");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("stone");

	m_basicMeshes->DrawTorusMesh();

	//Tire 3 Hubcap
	// set the XYZ scale of HC 3
	scaleXYZ = glm::vec3(0.08f, 0.08f, 0.08f);

	// set the XYZ rotation of HC 3
	// hubcap spins with the wheel as well
	XrotationDegrees = m_wheelAngle;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of HC 3
	positionXYZ = glm::vec3(2.5f, 0.4f, 1.15f);
	positionXYZ += glm::vec3(m_truckOffset, 0.0f, 0.0f); // move with truck

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(0, 0, 0, 1);
	SetShaderTexture("plastic");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("plastic");

	m_basicMeshes->DrawSphereMesh();


	//Tire 4 (Front Driver side tire)
	// set the XYZ scale of Front DS Tire
	scaleXYZ = glm::vec3(0.1f, 0.1f, 0.1f);

	// wheel spin should rotate the wheel around its local X axis (tweak axis if visually wrong)
	XrotationDegrees = m_wheelAngle;      // spin
	YrotationDegrees = 15.0f;			//existing tilt
	ZrotationDegrees = 0.0f;

	// set the XYZ position of Front DS Tire
	positionXYZ = glm::vec3(1.5f, 0.4f, 0.95f);
	positionXYZ += glm::vec3(m_truckOffset, 0.0f, 0.0f); // move with truck

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(0, 0, 0, 1);
	SetShaderTexture("chalk");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("stone");

	m_basicMeshes->DrawTorusMesh();

	//Tire 4 Hubcap
	// set the XYZ scale of HC 4
	scaleXYZ = glm::vec3(0.08f, 0.08f, 0.08f);

	// set the XYZ rotation of HC 4
	// hubcap spins with the wheel as well
	XrotationDegrees = m_wheelAngle;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of HC 4
	positionXYZ = glm::vec3(1.5f, 0.4f, 0.95f);
	positionXYZ += glm::vec3(m_truckOffset, 0.0f, 0.0f);	// move with truck

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(0, 0, 0, 1);
	SetShaderTexture("plastic");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("plastic");

	m_basicMeshes->DrawSphereMesh();


	//Create Truck Cab
	// set the XYZ scale of cab
	scaleXYZ = glm::vec3(0.3f, 0.5f, 0.3f);

	// set the XYZ rotation of cab
	XrotationDegrees = 0.0f;
	YrotationDegrees = -15.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of cab
	positionXYZ = glm::vec3(1.5f, 0.8f, 0.8f);
	positionXYZ += glm::vec3(m_truckOffset, 0.0f, 0.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(0, 0, 0, 1);
	SetShaderTexture("cab");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("plastic");

	m_basicMeshes->DrawBoxMesh();

	//Create Back of truck cab (Boxes)
	//Horizontal box
	// set the XYZ scale of HB
	scaleXYZ = glm::vec3(1.3f, 0.03f, 0.4f);

	// set the XYZ rotation of HB
	XrotationDegrees = 180.0f;
	YrotationDegrees = 12.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of HB
	positionXYZ = glm::vec3(2.0f, 0.5f, 0.8f);
	positionXYZ += glm::vec3(m_truckOffset, 0.0f, 0.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(0, 0, 0, 1);
	SetShaderTexture("sink");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("plastic");

	m_basicMeshes->DrawBoxMesh();

	//Vertical box
	// set the XYZ scale of VB
	scaleXYZ = glm::vec3(0.03f, 0.8f, 0.4f);

	// set the XYZ rotation of VB
	XrotationDegrees = 180.0f;
	YrotationDegrees = 12.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of VB
	positionXYZ = glm::vec3(1.65f, 0.95f, 0.8f);
	positionXYZ += glm::vec3(m_truckOffset, 0.0f, 0.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(0, 0, 0, 1);
	SetShaderTexture("sink");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("plastic");

	m_basicMeshes->DrawBoxMesh();

	//Create Cement Mixer Drum
	// set the XYZ scale of Drum
	scaleXYZ = glm::vec3(0.4f, 0.85f, 0.2f);

	// set the XYZ rotation of Drum
	XrotationDegrees = 0.0f;
	YrotationDegrees = -25.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position of Drum
	positionXYZ = glm::vec3(2.5f, 0.9f, 1.0f);
	positionXYZ += glm::vec3(m_truckOffset, 0.0f, 0.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(0, 0, 0, 1);
	SetShaderTexture("plastic");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("plastic");

	m_basicMeshes->DrawTaperedCylinderMesh();


	//FENCE BACKGROUND
	//Back fence
	// set the XYZ scale of fence background
	scaleXYZ = glm::vec3(14.0f, 14.0f, 0.2f);

	// set the XYZ rotation of fence
	XrotationDegrees = 0.0f;
	YrotationDegrees = -10.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of fence
	positionXYZ = glm::vec3(1.5f, 0.0f, -6.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 0, 1, 1);
	SetShaderTexture("fence");
	SetTextureUVScale(4.0f, 1.0f);
	SetShaderMaterial("wood");

	m_basicMeshes->DrawBoxMesh();

	//Left fence
	// set the XYZ scale of fence background
	scaleXYZ = glm::vec3(14.0f, 14.0f, 0.2f);

	// set the XYZ rotation of fence
	XrotationDegrees = 0.0f;
	YrotationDegrees = 80.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of fence
	positionXYZ = glm::vec3(-6.5f, 0.0f, -1.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 0, 1, 1);
	SetShaderTexture("fence");
	SetTextureUVScale(4.0f, 1.0f);
	SetShaderMaterial("wood");

	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/
}

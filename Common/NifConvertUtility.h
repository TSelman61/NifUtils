///////////////////////////////////////////////////////////
//  NifConvertUtility.h
//  Implementation of the Class NifConvertUtility
//  Created on:      06-Mai-2012 10:03:33
//  Original author: Skyfox
///////////////////////////////////////////////////////////

#if !defined(NIFCONVERTUTILITY_H)
#define NIFCONVERTUTILITY_H

//  common includes
#include <set>

//  NifUtiliy includes
#include "VertexColorHandling.h"

//  Niflib includes
#include "obj/nitrishape.h"
#include "obj/rootcollisionnode.h"
#include "obj/bsfadenode.h"
#include "obj/nialphaproperty.h"
#include "obj/NiTriShapeData.h"
#include "obj/BSLightingShaderProperty.h"

//  used namespaces
using namespace Niflib;
using namespace std;

//  return codes
#define   NCU_OK                            0x00
#define   NCU_ERROR_MISSING_FILE_NAME       0x01
#define   NCU_ERROR_MISSING_TEXTURE_NAME    0x02
#define   NCU_ERROR_CANT_OPEN_INPUT         0x03
#define   NCU_ERROR_CANT_OPEN_TEMPLATE      0x04
#define   NCU_ERROR_CANT_OPEN_OUTPUT        0x05
#define   NCU_ERROR_CANT_GET_GEOMETRY       0x06

//  message types
#define   NCU_MSG_TYPE_INFO                 0x00
#define   NCU_MSG_TYPE_WARNING              0x01
#define   NCU_MSG_TYPE_ERROR                0x02
#define   NCU_MSG_TYPE_TEXTURE              0x03
#define   NCU_MSG_TYPE_SUB_INFO             0x04
#define   NCU_MSG_TYPE_TEXTURE_MISS         0x05


namespace NifUtility
{
	/**
	 * @author Skyfox
	 * @version 1.0
	 * @created 06-Mai-2012 10:03:33
	 */
	class NifConvertUtility
	{

	public:
		/**
		 * Default Constructor
		 */
		NifConvertUtility();

		/**
		 * Destructor
		 */
		virtual ~NifConvertUtility();

		/**
		 * Convert shape part of NIF to new format
		 * 
		 * @param fileNameSrc    in: filename of NIF to be converted
		 * @param fileNameDst    in: filename of NIF to be produced
		 * @param fileNameTmpl    in: path and name of Nif-file used as template
		 */
		virtual unsigned int convertShape(string fileNameSrc, string fileNameDst, string fileNameTmpl);

		/**
		 * set texture path used for re-locate textures
		 * 
		 * @param pathTexture    in: path to texture base directory used for new location
		 * of textures
		 */
		virtual void setTexturePath(string pathTexture);

		/**
		 * set Skyrim path
		 * 
		 * @param pathSkyrim    in: path to Skyrim base directory
		 * of textures
		 */
		virtual void setSkyrimPath(string pathSkyrim);

		/**
		 * 
		 * @param vcHandling    in: handling of vertex colors when missing in source NIF
		 */
		virtual void setVertexColorHandling(VertexColorHandling vcHandling);

		/**
		 * 
		 * @param defaultColor    in: RGB color
		 */
		virtual void setDefaultVertexColor(Color4 defaultColor);

		/**
		 * 
		 * @param doUpdate    in: true: update tangent space
		 */
		virtual void setUpdateTangentSpace(bool doUpdate);

		/**
		 * 
		 * @param doReorder    in: true: reorder NiTriShape properties
		 */
		virtual void setReorderProperties(bool doReorder);

		/**
		 * Get list of user messages
		 */
		virtual vector<string>& getUserMessages();

		/**
		 * Get list of used textures
		 */
		virtual set<string>& getUsedTextures();

		/**
		 * Get list of non existing textures
		 */
		virtual set<string>& getNewTextures();

		/**
		 * Set callback function for logging info
		 */
		virtual void setLogCallback(void (*logCallback) (const int type, const char* pMessage));

	protected:

		void (*_logCallback) (const int, const char*);

		/**
		 * path to texture files
		 */
		string _pathTexture;

		/**
		 * path to Skyrim files
		 */
		string _pathSkyrim;

		/**
		 * log messages for user
		 */
		vector<string> _userMessages;

		/**
		 * list of used textures
		 */
		set<string> _usedTextures;

		/**
		 * list of non existing textures
		 */
		set<string> _newTextures;

		/**
		 * handling of vertex colors
		 */
		VertexColorHandling _vcHandling;

		/**
		 * default vertex color
		 */
		Color4 _vcDefaultColor;

		/**
		 * update tangent space
		 */
		bool _updateTangentSpace;

		/**
		 * reorder NiTriShape properties
		 */
		bool _reorderProperties;

		/**
		 * Get NiNode from NIF-file
		 * 
		 * @param fileName    in: path and name of NIF file
		 * @param logPreText    in: text prepended to log output
		 * @param fakedRoot    out: flag marking real root node or faked one
		 */
		virtual NiNodeRef getRootNodeFromNifFile(string fileName, string logPreText, bool& fakedRoot);

		/**
		 * Convert NiNode and all known sub-nodes
		 * 
		 * @param srcNode
		 * @param tmplNode
		 * @param rootNode    in: Root node of destination NIF tree
		 * @param tmplAlphaProp    tmplAlphaProp
		 */
		virtual NiNodeRef convertNiNode(NiNodeRef pSrcNode, NiTriShapeRef pTmplNode, NiNodeRef pRootNode, NiAlphaPropertyRef pTmplAlphaProp = NULL);

		/**
		 * Convert NiTriShape and properties/geometry
		 * 
		 * @param srcNode    in: Source NiTriShape node
		 * @param tmplNode    in: Template NiTriShape node
		 * @param tmplAlphaProp    in: Template for alpha properties
		 */
		virtual NiTriShapeRef convertNiTriShape(NiTriShapeRef pSrcNode, NiTriShapeRef pTmplNode, NiAlphaPropertyRef pTmplAlphaProp = NULL);

		/**
		 * Create tangent space data
		 * 
		 * @param pDataObj    in: data object
		 */
		virtual bool updateTangentSpace(NiTriShapeDataRef pDataObj);

		/**
		 * Clone BSLightingShaderProperty
		 * 
		 * @param pSource    in: ptr. to source object
		 */
		virtual BSLightingShaderPropertyRef cloneBSLightingShaderProperty(BSLightingShaderPropertyRef pSource);

		/**
		 * Log messages
		 * 
		 * @param type    in: message type
		 * @param text    in: message text
		 */
		virtual void logMessage(int type, string text);

		/**
		 * Check of existence of file
		 * 
		 * @param fileName    in: path and name of file to check
		 */
		virtual bool checkFileExists(string fileName);
	};

}
#endif // !defined(NIFCONVERTUTILITY_H)

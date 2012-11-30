///////////////////////////////////////////////////////////
//  NifConvertUtility.cpp
//  Implementation of the Class NifConvertUtility
//  Created on:      06-Mai-2012 10:03:33
//  Original author: Skyfox
///////////////////////////////////////////////////////////

//  Common includes
#include "NifConvertUtility.h"

//  Niflib includes
#include "niflib.h"
#include "obj/NiExtraData.h"
#include "obj/NiTimeController.h"
#include "obj/NiTexturingProperty.h"
#include "obj/BSShaderTextureSet.h"
#include "obj/NiSourceTexture.h"
#include "obj/NiMaterialProperty.h"

//  used namespaces
using namespace NifUtility;
using namespace Niflib;

/*---------------------------------------------------------------------------*/
NifConvertUtility::NifConvertUtility()
	:	_vcDefaultColor    (1.0f, 1.0f, 1.0f, 1.0f),
		_vcHandling        (NCU_VC_REMOVE_FLAG),
		_updateTangentSpace(true),
		_reorderProperties (true),
		_logCallback       (NULL)
{
}

/*---------------------------------------------------------------------------*/
NifConvertUtility::~NifConvertUtility()
{
}

/*---------------------------------------------------------------------------*/
NiNodeRef NifConvertUtility::getRootNodeFromNifFile(string fileName, string logPreText, bool& fakedRoot)
{
	NiObjectRef		pRootTree (NULL);
	NiNodeRef		pRootInput(NULL);

	//  get input nif
	pRootTree = ReadNifTree((const char*) fileName.c_str());

	//  NiNode as root
	if (DynamicCast<NiNode>(pRootTree) != NULL)
	{
		pRootInput = DynamicCast<NiNode>(pRootTree);
		logMessage(NCU_MSG_TYPE_INFO, logPreText + " root is NiNode.");
	}
	//  NiTriShape as root
	else if (DynamicCast<NiTriShape>(pRootTree) != NULL)
	{
		//  create faked root
		pRootInput = new NiNode();

		//  add root as child
		pRootInput->AddChild(DynamicCast<NiAVObject>(pRootTree));

		//  mark faked root node
		fakedRoot = true;

		logMessage(NCU_MSG_TYPE_INFO, logPreText + " root is NiTriShape.");
	}

	//  no known root type found
	if (pRootInput == NULL)
	{
		logMessage(NCU_MSG_TYPE_WARNING, logPreText + " root has unhandled type: " + pRootTree->GetType().GetTypeName());
	}

	return pRootInput;
}

/*---------------------------------------------------------------------------*/
NiNodeRef NifConvertUtility::convertNiNode(NiNodeRef pSrcNode, NiTriShapeRef pTmplNode, NiNodeRef pRootNode, NiAlphaPropertyRef pTmplAlphaProp)
{
	NiNodeRef				pDstNode    (pSrcNode);
	vector<NiAVObjectRef>	srcShapeList(pDstNode->GetChildren());

	//  find NiAlphaProperty and use as template in sub-nodes
	if (DynamicCast<NiAlphaProperty>(pDstNode->GetPropertyByType(NiAlphaProperty::TYPE)) != NULL)
	{
		pTmplAlphaProp = DynamicCast<NiAlphaProperty>(pDstNode->GetPropertyByType(NiAlphaProperty::TYPE));
	}

	//  unlink protperties -> not used in new format
	pDstNode->ClearProperties();

	//  shift extra data to new version
	pDstNode->ShiftExtraData(VER_20_2_0_7);

	//  unlink children
	pDstNode->ClearChildren();

	//  iterate over source nodes and convert using template
	for (vector<NiAVObjectRef>::iterator  ppIter = srcShapeList.begin(); ppIter != srcShapeList.end(); ppIter++)
	{
		//  NiTriShape
		if (DynamicCast<NiTriShape>(*ppIter) != NULL)
		{
			pDstNode->AddChild(&(*convertNiTriShape(DynamicCast<NiTriShape>(*ppIter), pTmplNode, pTmplAlphaProp)));
		}
		//  NiNode (and derived classes?)
		else if (DynamicCast<NiNode>(*ppIter) != NULL)
		{
			pDstNode->AddChild(&(*convertNiNode(DynamicCast<NiNode>(*ppIter), pTmplNode, pRootNode, pTmplAlphaProp)));
		}
	}

	return pDstNode;
}

/*---------------------------------------------------------------------------*/
NiTriShapeRef NifConvertUtility::convertNiTriShape(NiTriShapeRef pSrcNode, NiTriShapeRef pTmplNode, NiAlphaPropertyRef pTmplAlphaProp)
{
	BSLightingShaderPropertyRef	pTmplLShader(NULL);
	BSLightingShaderPropertyRef	pDstLShader (NULL);
	vector<NiPropertyRef>		dstPropList;
	short						bsPropIdx   (0);
	bool						forceAlpha  (pTmplAlphaProp != NULL);
	bool						hasAlpha    (false);

	//  NiTriShape is moved from src to dest. It's unlinked in calling function
	NiTriShapeRef		pDstNode(pSrcNode);
	NiGeometryDataRef	pDstGeo (pDstNode->GetData());

	//  force some data in destination shape
	pDstNode->SetCollisionObject(NULL);  //  no collision object here
	pDstNode->SetFlags          (14);    //  ???

	//  data node
	if (pDstGeo != NULL)
	{
		//  set flags
		if (pTmplNode->GetData() != NULL)
		{
			pDstGeo->SetConsistencyFlags(pTmplNode->GetData()->GetConsistencyFlags());  //  nessessary ???
		}

		//  update tangent space?
		if ((_updateTangentSpace) && (DynamicCast<NiTriShapeData>(pDstGeo) != NULL))
		{
			//  update tangent space
			if (updateTangentSpace(DynamicCast<NiTriShapeData>(pDstGeo)))
			{
				//  enable tangent space
				pDstGeo->SetTspaceFlag(0x10);
			}
		}  //  if (_updateTangentSpace)
	}  //  if (pDstGeo != NULL)

	//  properties - get them from template
	for (short index(0); index < 2; ++index)
	{
		//  BSLightingShaderProperty
		if (DynamicCast<BSLightingShaderProperty>(pTmplNode->GetBSProperty(index)) != NULL)
		{
			pTmplLShader = DynamicCast<BSLightingShaderProperty>(pTmplNode->GetBSProperty(index));
		}
		//  NiAlphaProperty
		else if (DynamicCast<NiAlphaProperty>(pTmplNode->GetBSProperty(index)) != NULL)
		{
			pTmplAlphaProp = DynamicCast<NiAlphaProperty>(pTmplNode->GetBSProperty(index));
		}
	}  //  for (short index(0); index < 2; ++index)

	//  parse properties of destination node
	dstPropList = pDstNode->GetProperties();

	for (vector<NiPropertyRef>::iterator  ppIter = dstPropList.begin(); ppIter != dstPropList.end(); ppIter++)
	{
		//  NiAlphaProperty
		if (DynamicCast<NiAlphaProperty>(*ppIter) != NULL)
		{
			NiAlphaPropertyRef	pPropAlpha(DynamicCast<NiAlphaProperty>(*ppIter));

			//  set values from template
			pPropAlpha->SetFlags        (pTmplAlphaProp->GetFlags());
			pPropAlpha->SetTestThreshold(pTmplAlphaProp->GetTestThreshold());

			//  remove property from node
			pDstNode->RemoveProperty(*ppIter);

			//  set new property to node
			pDstNode->SetBSProperty(bsPropIdx++, &(*pPropAlpha));

			//  own alpha, reset forced alpha
			forceAlpha = false;

			//  mark alpha property
			hasAlpha = true;
		}
		//  NiTexturingProperty
		else if (DynamicCast<NiTexturingProperty>(*ppIter) != NULL)
		{
			char*						pTextPos (NULL);
			BSShaderTextureSetRef		pDstSText(new BSShaderTextureSet());
			TexDesc						baseTex  ((DynamicCast<NiTexturingProperty>(*ppIter))->GetTexture(BASE_MAP));
			char						fileName[1000] = {0};
			char						textName[1000] = {0};

			//  clone shader property from template
			pDstLShader = cloneBSLightingShaderProperty(pTmplLShader);
			//pDstLShader = new BSLightingShaderProperty();

			//  copy textures from template to copy
			pDstSText->SetTextures(pTmplLShader->GetTextureSet()->GetTextures());

			//  set new texture names
			sprintf(fileName, "%s", (const char*) _pathTexture.c_str());
			baseTex.source->GetTextureFileName().copy(textName, 1000, 0);
			pTextPos = strrchr(textName, '\\');
			if (pTextPos == NULL)
			{
				pTextPos = strrchr(textName, '/');
			}
			if (pTextPos != NULL)
			{
				strcat(fileName, ++pTextPos);
			}
			else
			{
				strcat(fileName, textName);
			}
			fileName[strlen(fileName) - 3] = 0;
			strcat(fileName, "dds");

			//  set new texture map
			pDstSText->SetTexture(0, fileName);

			logMessage(NCU_MSG_TYPE_TEXTURE, string("Txt-Used: ") + fileName);
			if (!checkFileExists(fileName))
			{
				_newTextures.insert(string("Txt-Missed: ") + fileName);
			}

			fileName[strlen(fileName) - 4] = 0;
			strcat(fileName, "_n.dds");

			//  set new normal map
			pDstSText->SetTexture(1, fileName);

			if (!checkFileExists(fileName))
			{
				_newTextures.insert(string("Txt-Missed: ") + fileName);
			}

			//  add texture set to texture property
			pDstLShader->SetTextureSet(pDstSText);

			//  check for existing vertex colors
			if ((pDstGeo != NULL) && (pDstGeo->GetColors().size() <= 0) && ((pDstLShader->GetShaderFlags2() & Niflib::SLSF2_VERTEX_COLORS) != 0))
			{
				switch (_vcHandling)
				{
					case NCU_VC_REMOVE_FLAG:
					{
						pDstLShader->SetShaderFlags2((SkyrimShaderPropertyFlags2) (pDstLShader->GetShaderFlags2() & ~Niflib::SLSF2_VERTEX_COLORS));
						break;
					}

					case NCU_VC_ADD_DEFAULT:
					{
						pDstGeo->SetVertexColors(vector<Color4>(pDstGeo->GetVertexCount(), _vcDefaultColor));
						break;
					}
				}
			}  //  if ((pDstGeo != NULL) && (pDstGeo->GetColors().size() <= 0) && ...

			//  remove property from node
			pDstNode->RemoveProperty(*ppIter);

			//  set new property to node
			pDstNode->SetBSProperty(bsPropIdx++, &(*pDstLShader));
		}
		//  NiMaterialProperty
		else if (DynamicCast<NiMaterialProperty>(*ppIter) != NULL)
		{
			//  remove property from node
			pDstNode->RemoveProperty(*ppIter);
		}
	}  //  for (vector<NiPropertyRef>::iterator  ppIter = dstPropList.begin(); ppIter != dstPropList.end(); ppIter++)

	//  add forced NiAlphaProperty?
	if (forceAlpha)
	{
		NiAlphaPropertyRef	pPropAlpha(new NiAlphaProperty());

		//  set values from template
		pPropAlpha->SetFlags        (pTmplAlphaProp->GetFlags());
		pPropAlpha->SetTestThreshold(pTmplAlphaProp->GetTestThreshold());

		//  set new property to node
		pDstNode->SetBSProperty(bsPropIdx++, &(*pPropAlpha));

		//  mark alpha property
		hasAlpha = true;

	}  //  if (forceAlpha)

	//  add default vertex colors if alpha property and no colors
	if (hasAlpha && (pDstGeo != NULL) && (pDstGeo->GetColors().size() <= 0))
	{
		pDstGeo->SetVertexColors(vector<Color4>(pDstGeo->GetVertexCount(), _vcDefaultColor));

		//  force flag in BSLightingShaderProperty
		if (pDstLShader != NULL)
		{
			pDstLShader->SetShaderFlags2((SkyrimShaderPropertyFlags2) (pDstLShader->GetShaderFlags2() | Niflib::SLSF2_VERTEX_COLORS));
		}
	}

	//  reorder properties
	if (_reorderProperties)
	{
		NiPropertyRef	tProp01(pDstNode->GetBSProperty(0));
		NiPropertyRef	tProp02(pDstNode->GetBSProperty(1));

		//  make sure BSLightingShaderProperty comes before NiAlphaProperty - seems a 'must be'
		if ((tProp01->GetType().GetTypeName() == "NiAlphaProperty") &&
			(tProp02->GetType().GetTypeName() == "BSLightingShaderProperty")
		   )
		{
			pDstNode->SetBSProperty(0, tProp02);
			pDstNode->SetBSProperty(1, tProp01);
		}
	}  //  if (_reorderProperties)

	return  pDstNode;
}

/*---------------------------------------------------------------------------*/
unsigned int NifConvertUtility::convertShape(string fileNameSrc, string fileNameDst, string fileNameTmpl)
{
	NiNodeRef				pRootInput     (NULL);
	NiNodeRef				pRootOutput    (NULL);
	NiNodeRef				pRootTemplate  (NULL);
	NiTriShapeRef			pNiTriShapeTmpl(NULL);
	vector<NiAVObjectRef>	srcChildList;
	bool					fakedRoot      (false);

	//  test on existing file names
	if (fileNameSrc.empty())		return NCU_ERROR_MISSING_FILE_NAME;
	if (fileNameDst.empty())		return NCU_ERROR_MISSING_FILE_NAME;
	if (fileNameTmpl.empty())		return NCU_ERROR_MISSING_FILE_NAME;

	//  initialize user messages
	_userMessages.clear();
	logMessage(NCU_MSG_TYPE_INFO, "Source:  "      + (fileNameSrc.empty() ? "- none -" : fileNameSrc));
	logMessage(NCU_MSG_TYPE_INFO, "Template:  "    + (fileNameTmpl.empty() ? "- none -" : fileNameTmpl));
	logMessage(NCU_MSG_TYPE_INFO, "Destination:  " + (fileNameDst.empty() ? "- none -" : fileNameDst));
	logMessage(NCU_MSG_TYPE_INFO, "Texture:  "     + (_pathTexture.empty() ? "- none -" : _pathTexture));

	//  initialize used texture list
	_usedTextures.clear();
	_newTextures.clear();

	//  read input NIF
	if ((pRootInput = getRootNodeFromNifFile(fileNameSrc, "source", fakedRoot)) == NULL)
	{
		logMessage(NCU_MSG_TYPE_ERROR, "Can't open '" + fileNameSrc + "' as input");
		return NCU_ERROR_CANT_OPEN_INPUT;
	}

	//  get template nif
	pRootTemplate = DynamicCast<BSFadeNode>(ReadNifTree((const char*) fileNameTmpl.c_str()));
	if (pRootTemplate == NULL)
	{
		logMessage(NCU_MSG_TYPE_ERROR, "Can't open '" + fileNameTmpl + "' as template");
		return NCU_ERROR_CANT_OPEN_TEMPLATE;
	}

	//  get shapes from template
	//  - shape root
	pNiTriShapeTmpl = DynamicCast<NiTriShape>(pRootTemplate->GetChildren().at(0));
	if (pNiTriShapeTmpl == NULL)
	{
		logMessage(NCU_MSG_TYPE_INFO, "Template has no NiTriShape.");
	}

	//  template root is used as root of output
	pRootOutput = pRootTemplate;

	//   get rid of unwanted subnodes
	pRootOutput->ClearChildren();           //  remove all children
	pRootOutput->SetCollisionObject(NULL);  //  unlink collision object
	//  hold extra data and property nodes

	//  copy translation from input node
	pRootOutput->SetLocalTransform(pRootInput->GetLocalTransform());

	//  copy name of root node
	pRootOutput->SetName(pRootInput->GetName());

	//  get list of children from input node
	srcChildList = pRootInput->GetChildren();

	//  unlink children 'cause moved to output
	pRootInput->ClearChildren();

	//  iterate over source nodes and convert using template
	for (vector<NiAVObjectRef>::iterator  ppIter = srcChildList.begin(); ppIter != srcChildList.end(); ppIter++)
	{
		//  NiTriShape
		if (DynamicCast<NiTriShape>(*ppIter) != NULL)
		{
			pRootOutput->AddChild(&(*convertNiTriShape(DynamicCast<NiTriShape>(*ppIter), pNiTriShapeTmpl)));
		}
		//  RootCollisionNode
		else if (DynamicCast<RootCollisionNode>(*ppIter) != NULL)
		{
			//  ignore node
		}
		//  NiNode (and derived classes?)
		else if (DynamicCast<NiNode>(*ppIter) != NULL)
		{
			pRootOutput->AddChild(&(*convertNiNode(DynamicCast<NiNode>(*ppIter), pNiTriShapeTmpl, pRootOutput)));
		}
	}

	//  write missing textures to log - as block
	for (set<string>::iterator pIter(_newTextures.begin()); pIter != _newTextures.end(); ++pIter)
	{
		logMessage(NCU_MSG_TYPE_TEXTURE_MISS, *pIter);
	}

	//  write modified nif file
	WriteNifTree((const char*) fileNameDst.c_str(), pRootOutput, NifInfo(VER_20_2_0_7, 12, 83));

	return NCU_OK;
}

/*---------------------------------------------------------------------------*/
bool NifConvertUtility::updateTangentSpace(NiTriShapeDataRef pDataObj)
{
	vector<Vector3>		vecVertices (pDataObj->GetVertices());
	vector<Vector3>		vecNormals  (pDataObj->GetNormals());
	vector<TexCoord>	vecTexCoords(pDataObj->GetUVSet(0));
	vector<Triangle>	vecTriangles(pDataObj->GetTriangles());

	//  check on valid input data
	if (vecVertices.empty() || vecTriangles.empty() || vecNormals.size() != vecVertices.size() || vecVertices.size() != vecTexCoords.size())
	{
		logMessage(NCU_MSG_TYPE_INFO, "UpdateTangentSpace: No vertices, normals, coords or faces defined.");
		return false;
	}

	//  prepare result vectors
	vector<Vector3>		vecTangents  = vector<Vector3>(vecVertices.size(), Vector3(0.0f, 0.0f, 0.0f));
	vector<Vector3>		vecBiNormals = vector<Vector3>(vecVertices.size(), Vector3(0.0f, 0.0f, 0.0f));

	for (unsigned int t(0); t < vecTriangles.size(); ++t)
	{
		Vector3		vec21(vecVertices[vecTriangles[t][1]] - vecVertices[vecTriangles[t][0]]);
		Vector3		vec31(vecVertices[vecTriangles[t][2]] - vecVertices[vecTriangles[t][0]]);
		TexCoord	txc21(vecTexCoords[vecTriangles[t][1]] - vecTexCoords[vecTriangles[t][0]]);
		TexCoord	txc31(vecTexCoords[vecTriangles[t][2]] - vecTexCoords[vecTriangles[t][0]]);
		float		radius(((txc21.u * txc31.v - txc31.u * txc21.v) >= 0.0f ? +1.0f : -1.0f));

		Vector3		sdir(( txc31.v * vec21[0] - txc21.v * vec31[0] ) * radius,
					     ( txc31.v * vec21[1] - txc21.v * vec31[1] ) * radius,
					     ( txc31.v * vec21[2] - txc21.v * vec31[2] ) * radius);
		Vector3		tdir(( txc21.u * vec31[0] - txc31.u * vec21[0] ) * radius,
					     ( txc21.u * vec31[1] - txc31.u * vec21[1] ) * radius,
					     ( txc21.u * vec31[2] - txc31.u * vec21[2] ) * radius);

		//  normalize
		sdir = sdir.Normalized();
		tdir = tdir.Normalized();

		for (int j(0); j < 3; ++j)
		{
			vecTangents [vecTriangles[t][j]] += tdir;
			vecBiNormals[vecTriangles[t][j]] += sdir;
		}
	}  //  for (unsigned int t(0); t < vecTriangles.size(); ++t)

	for (unsigned int i(0); i < vecVertices.size(); ++i)
	{
		Vector3&	n(vecNormals[i]);
		Vector3&	t(vecTangents [i]);
		Vector3&	b(vecBiNormals[i]);

		if ((t == Vector3()) || (b == Vector3()))
		{
			t[0] = n[1];
			t[1] = n[2];
			t[2] = n[0];

			b = n.CrossProduct(t);
		}
		else
		{
			t = t.Normalized();
			t = (t - n * n.DotProduct(t));
			t = t.Normalized();

			b = b.Normalized();
			b = (b - n * n.DotProduct(b));
			b = (b - t * t.DotProduct(b));
			b = b.Normalized();
		}
	}  //  for (unsigned int i(0); i < vecVertices.size(); ++i)

	//  set tangents and binormals to object
	pDataObj->SetBitangents(vecBiNormals);
	pDataObj->SetTangents  (vecTangents);

	return true;
}

/*---------------------------------------------------------------------------*/
void NifConvertUtility::setTexturePath(string pathTexture)
{
	_pathTexture = pathTexture;
}

/*---------------------------------------------------------------------------*/
void NifConvertUtility::setSkyrimPath(string pathSkyrim)
{
	_pathSkyrim = pathSkyrim;
}

/*---------------------------------------------------------------------------*/
void NifConvertUtility::setVertexColorHandling(VertexColorHandling vcHandling)
{
	_vcHandling = vcHandling;
}

/*---------------------------------------------------------------------------*/
void NifConvertUtility::setDefaultVertexColor(Color4 defaultColor)
{
	_vcDefaultColor = defaultColor;
}

/*---------------------------------------------------------------------------*/
void NifConvertUtility::setUpdateTangentSpace(bool doUpdate)
{
	_updateTangentSpace = doUpdate;
}

/*---------------------------------------------------------------------------*/
void NifConvertUtility::setReorderProperties(bool doReorder)
{
	_reorderProperties = doReorder;
}

/*---------------------------------------------------------------------------*/
vector<string>& NifConvertUtility::getUserMessages()
{
	return _userMessages;
}

/*---------------------------------------------------------------------------*/
set<string>& NifConvertUtility::getUsedTextures()
{
	return _usedTextures;
}

/*---------------------------------------------------------------------------*/
set<string>& NifConvertUtility::getNewTextures()
{
	return _newTextures;
}

/*---------------------------------------------------------------------------*/
void NifConvertUtility::setLogCallback(void (*logCallback) (const int type, const char* pMessage))
{
	_logCallback = logCallback;
}

/*---------------------------------------------------------------------------*/
void NifConvertUtility::logMessage(int type, string text)
{
	//  add message to internal storages
	switch (type)
	{
		default:
		case NCU_MSG_TYPE_INFO:
		case NCU_MSG_TYPE_WARNING:
		case NCU_MSG_TYPE_ERROR:
		{
			_userMessages.push_back(text);
			break;
		}

		case NCU_MSG_TYPE_TEXTURE:
		{
			_usedTextures.insert(text);
			break;
		}

		case NCU_MSG_TYPE_TEXTURE_MISS:
		{
//			_newTextures.insert(text);
			break;
		}
	}

	//  send message to callback if given
	if (_logCallback != NULL)
	{
		(*_logCallback)(type, text.c_str());
	}
}

/*---------------------------------------------------------------------------*/
bool NifConvertUtility::checkFileExists(string fileName)
{
	string		path   (_pathSkyrim + "\\Data\\" + fileName);
	ifstream	iStream(path.c_str());

	//  return existance of file
	return iStream.good();
}

/*---------------------------------------------------------------------------*/
BSLightingShaderPropertyRef NifConvertUtility::cloneBSLightingShaderProperty(BSLightingShaderPropertyRef pSource)
{
	BSLightingShaderPropertyRef		pDest(new BSLightingShaderProperty);
	BSShaderTextureSetRef			pTSet(pSource->GetTextureSet());

	//  force empty texture set to source (HACK)
	pSource->SetTextureSet(NULL);

	//  copy all members, even inaccessable ones (HACK)
	memcpy(pDest, pSource, sizeof(BSLightingShaderProperty));

	//  reset source texture set
	pSource->SetTextureSet(pTSet);

	return pDest;
}

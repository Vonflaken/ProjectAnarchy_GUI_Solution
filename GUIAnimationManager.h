#ifndef GUIANIMATIONMANAGER_H_INCLUDED
#define GUIANIMATIONMANAGER_H_INCLUDED


#include "GlobalTypes.h"
#include "GUIAnimation.h"
#include <string>
#include <map>
#include <sstream>


#define AUTO_LOAD_HD_TEX false
#define MAX_W_OR_H_SD 960
#define MAX_W_OR_H_HD 2048
#define TPTEXFILE_EXTENSION ".png"

class GUIAnimationManager
{
	friend class GUIAnimation;

protected:
	GUIAnimationManager();


public:
	virtual ~GUIAnimationManager();

	static GUIAnimationManager& Instance() { if ( !s_pInstance ) s_pInstance = new GUIAnimationManager(); return *s_pInstance; }
	static void DeInit() { if ( s_pInstance ) delete s_pInstance; s_pInstance = 0; }


	GUIAnimation* CreateAnimation( const std::string& sSrcTexFilepathWithoutExtension, const std::string& _sFilename, const int iFirstFrame, const int iLastFrame, const int iNumFrames, const eGUIAnimID eID, const GUIAnimation::eGUIAnimType eType = GUIAnimation::GAT_ONCE );

	bool IsValid() const { return m_bIsValid; }

	void AddAnimation( GUIAnimation* guiAnimation );
	void RemoveAnimation( GUIAnimation* guiAnimation );
	GUIAnimation* GetAnimation( unsigned int eAnimID );
	void Update( float fDeltaTime );
	std::map< std::string, GUIAnimation::FrameRect >& GetMap() { return m_hDecodedTexturePackerJSON; }

	void SetElement( const std::string& sKey, const GUIAnimation::FrameRect& tValue ) { m_hDecodedTexturePackerJSON[sKey] = tValue; }
	const GUIAnimation::FrameRect& GetFrame( const std::string& sKey ) { return m_hDecodedTexturePackerJSON[sKey]; }

	VInputMap* GetInputMap() { return m_pInputMap; }
	bool LoadTexturePackerJSON( const std::string& sFilenameWithoutExtension, const std::string& sPath );

	const std::string& GetHDExtension() const { return m_sHDExtension; }
	bool IsHD() const { return m_bIsHD; }

private:
	static GUIAnimationManager* s_pInstance;

	DynArray_cl<GUIAnimation*> m_aAnimations;
	std::map< std::string, GUIAnimation::FrameRect > m_hDecodedTexturePackerJSON;
	VInputMap* m_pInputMap;
	bool m_bIsValid;
	std::string m_sHDExtension;
	bool m_bIsHD;
	GUIAnimation* m_pTouchHandler;
	bool m_bAnimsArrayIsDirty;
};


#endif // GUIANIMATIONMANAGER_H_INCLUDED
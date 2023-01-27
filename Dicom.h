/*
  Copyright 1993-2008 Medical Image Processing Group
              Department of Radiology
            University of Pennsylvania

This file is part of CAVASS.

CAVASS is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

CAVASS is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with CAVASS.  If not, see <http://www.gnu.org/licenses/>.

*/

//======================================================================
/**
 * \file   Dicom.h
 * \brief  DICOM dictionary and DICOM data classes.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#ifndef __Dicom_h
#define __Dicom_h

#if defined (WIN32) || defined (_WIN32)
    #pragma warning(disable:4786)  //necessary because stl generates longer
                                   // names than bill's compiler can handle!
#endif

#include  <map>
#include  <string>
//======================================================================
/**
 * \brief Definition and implementation of an entry in the DICOM dictionary.
 */
class DicomDictionaryEntry {
public:
    const char* const  mGroup;    ///< hex ascii string
    const char* const  mElement;  ///< hex ascii string
    const char* const  mVers;     ///< version
    const char* const  mVr;       ///< value representation
    const char* const  mVm;       ///< value multiplicity
    const char* const  mKeyword;  ///< shorter description
    const char* const  mName;     ///< longer description
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    DicomDictionaryEntry ( const char* const group, const char* const element,
        const char* const vers, const char* const vr, const char* const vm,
        const char* const keyword, const char* const name )
        : mGroup(group), mElement(element), mVers(vers), mVr(vr), mVm(vm),
          mKeyword(keyword), mName(name)
    {
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    friend std::ostream& operator<< ( std::ostream &out,
                                      const DicomDictionaryEntry& e );
};
//======================================================================
/**
 * \brief Definition and implementation of the DICOM dictionary.
 */
class DicomDictionary {
public:
    static std::map< std::string, DicomDictionaryEntry* >  m;

    static const int  bitsAllocated[2];               ///< = { group, element }
    static const int  bitsStored[2];                  ///< = { group, element }
    static const int  columns[2];                     ///< = { group, element }
    static const int  iconImageSequence[2];           ///< = { group, element }
    static const int  item[2];                        ///< = { group, element }
    static const int  itemDelimitationItem[2];        ///< = { group, element }
    static const int  manufacturer[2];                ///< = { group, element }
    static const int  numberOfFrames[2];              ///< = { group, element }
    static const int  pixelData[2];                   ///< = { group, element }
    static const int  pixelRepresentation[2];         ///< = { group, element }
    static const int  pixelSpacing[2];                ///< = { group, element }
    static const int  rescaleIntercept[2];            ///< = { group, element }
    static const int  rescaleSlope[2];                ///< = { group, element }
    static const int  rows[2];                        ///< = { group, element }
    static const int  samplesPerPixel[2];             ///< = { group, element }
    static const int  sequenceDelimitationItem[2];    ///< = { group, element }
    static const int  variablePixelData[2];           ///< = { group, element }
    static const int  windowCenter[2];                ///< = { group, element }
    static const int  windowWidth[2];                 ///< = { group, element }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    DicomDictionary ( const bool verbose=false );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    //Note:  The following function must be kept in sync with checkPattern
    // below.
    static DicomDictionaryEntry* getEntry ( const int group,
                                            const int element )
    {
        if (group==-1 && element==-1)    return NULL;
        char  key[255];

        //check "########"
        snprintf( key, sizeof key, "%04X%04X", group, element );
        if (m[key]!=NULL)    return m[key];

        assert(strlen(key)==8);  //the checks below assume this length

        //check "######XX"
        snprintf( key, sizeof key, "%04X%04X", group, element );
        key[6] = key[7] = 'X';
        if (m[key]!=NULL)    return m[key];

        //check "######X#"
        snprintf( key, sizeof key, "%04X%04X", group, element );
        key[6] = 'X';
        if (m[key]!=NULL)    return m[key];

        //check "####XXXX"
        snprintf( key, sizeof key, "%04X%04X", group, element );
        key[4] = key[5] = key[6] = key[7] = 'X';
        if (m[key]!=NULL)    return m[key];

        //check "##XX####"
        snprintf( key, sizeof key, "%04X%04X", group, element );
        key[2] = key[3] = 'X';
        return m[key];  //null will be returned if not found.
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    friend std::ostream& operator<< ( std::ostream &out,
                                      const DicomDictionary& d );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    static bool flippedGroup ( const int group ) {
        //this function determines if the bytes of the given group might possibly
        //be flipped.  this is useful because a weird example (produced by a GE
        //ultrasound machine) was submitted to me that all of sudden decided to
        //change the endian-ness in the middle of the input stream (without warning
        //and for no obvious good reason).
        switch (group) {
            case 0x0200 :
            case 0x0800 :
            //case 0x1000 :
            case 0x1800 :
            //case 0x2000 :
            case 0x2800 :
            //case 0x5400 :
            case 0x1020 :
            //case 0x0040 :
            case 0xe07f :
            case 0xfeff :  return true;
        }
       return false;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
private:
    //Note:  The following function must be kept in sync with getEntry 
    // above.
    static bool checkPattern ( std::string s ) {
        // replace each digit with '#'
        for (unsigned int i=0; i<s.length(); i++) {
            if (isxdigit(s[i]))    s[i]='#';
        }
        if (s=="########" || s=="######XX" || s=="######X#" ||
            s=="####XXXX" || s=="##XX####")    return true;
        return false;
    }

};
//======================================================================
/**
 * \brief Definition of a DICOM data element.
 */
class DicomDataElement {
public:
#if 0
    static const int   unknownType;  ///< don't know type!
    static const int   rootType;     ///< root type
    static const int   cType;        ///< character type
    static const int   sType;        ///< short type
    static const int   usType;       ///< unsigned short type
    static const int   iType;        ///< integer type
    static const int   uiType;       ///< unsigned integer type
    static const int   fType;        ///< float type
    static const int   dType;        ///< double type
#else
    enum { unknownType,  ///< don't know type!
           rootType,     ///< root type
           cType,        ///< character type
           ucType,       ///< unsigned character type
           sType,        ///< short type
           usType,       ///< unsigned short type
           iType,        ///< integer type
           uiType,       ///< unsigned integer type
           fType,        ///< float type
           dType         ///< double type
    };
#endif
    static const int   mMaxPrint;    ///< max items to print
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    int                iGroup;
    int                iElement;
    std::string        sVr;
    int                iLength;  //length in bytes
    int                iCount;   //number of a given type (NOT bytes)
    int                iWhichType;
    char*              cData;
    unsigned char*     ucData;
    short*             sData;
    unsigned short*    usData;
    int*               iData;
    unsigned int*      uiData;
    float*             fData;
    double*            dData;
    long               mOffset;  //byte offset from the beginning of the file
    //to create a tree:
    DicomDataElement*  mParent;     //pointer to parent
    DicomDataElement*  mSibling;    //pointer to sibling
    DicomDataElement*  mChild;      //pointer to first child
    DicomDataElement*  mLastChild;  //point to last child (for quick 
                                    // insertion at the end of the list)
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
public:
    DicomDataElement ( );
    void addSibling ( DicomDataElement* newNode );
    void addChild   ( DicomDataElement* newNode );
    friend std::ostream& operator<< ( std::ostream &out, const DicomDataElement& d );
    std::ostream& printTree ( std::ostream& out, int indent=0 ) const;
};
//======================================================================
/**
 * \brief Definition of a DICOM reader.
 */
class DicomReader {
public:
    DicomDataElement  mRoot;
private:
    FILE*             mFp;
    bool              mLittleEndian;
    bool              mExplicitVR;
    bool              mParseSQ;
    bool              mFlipBytes;

    static int        mMaxLength;
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    inline int read8 ( void ) {
        //this function reads a 8-bit entity/value.
        unsigned char  t1;
        int  n = fread(&t1, sizeof t1, 1, mFp);
        if (n!=1)  throw "";
        return t1;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    inline int read16 ( void ) {
        //this function reads an unsigned 16-bit entity/value.
        // -1 indicates eof.
        unsigned char  t1, t2;
        int  n = fread(&t1, sizeof t1, 1, mFp);
        if (n!=1)  return -1;
        n = fread(&t2, sizeof t2, 1, mFp);
        if (n!=1)  return -1;

        if (mLittleEndian)  return t1 + t2*256;
        return t1*256 + t2;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    inline int read32 ( void ) {
        //this function reads a 32-bit entity/value.
        unsigned char  t1, t2, t3, t4;
        int  n = fread(&t1, sizeof t1, 1, mFp);
        if (n!=1)  throw "";
        n = fread(&t2, sizeof t2, 1, mFp);
        if (n!=1)  throw "";
        n = fread(&t3, sizeof t3, 1, mFp);
        if (n!=1)  throw "";
        n = fread(&t4, sizeof t4, 1, mFp);
        if (n!=1)  throw "";

        if (mLittleEndian)  return t1 + t2*256 + t3*256*256 + t4*256*256*256;
        return t1*256*256*256 + t2*256*256 + t3*256 + t4;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    inline float readFloat ( void ) {
        float  f;
        int n = fread(&f, sizeof f, 1, mFp);
        if (n!=1)    throw "";
        return f;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    inline double readDouble ( void ) {
        double  d;
        int n = fread(&d, sizeof d, 1, mFp);
        if (n!=1)    throw "";
        return d;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
public:
    DicomReader ( const char* const fname, const DicomDictionary dd,
                  const bool verbose=false );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    static DicomDataElement* findEntry ( DicomDataElement* root,
        const int group, const int element, DicomDataElement* last=NULL )
    {
        //perform a breadth first search of the tree beginning with 'root'.
        // if 'last' is null, then return the first match that we find.
        // if 'last' is not null, then we have to keep searching until
        //     we find 'last' and then find a match after 'last'.
        if (root==NULL)    return NULL;
        if (root->iGroup==group && root->iElement==element) {
            if (last==NULL)    return root;  //first match
            if (root==last) {
                //found the last one.  keep searching and find the next one
                // and return it.
                DicomDataElement*  dde = findEntry(root->mSibling, group, element, NULL);
                if (dde!=NULL)    return dde;
                //either we found it or we didn't
                return findEntry(root->mChild, group, element, NULL);
            }
            //haven't found the last one yet.  keep searching.
            DicomDataElement*  dde = findEntry(root->mSibling, group, element, last);
            if (dde!=NULL)    return dde;
            //either we found it or we didn't
            return findEntry(root->mChild, group, element, last);
        }

        //haven't found a match yet.  keep searching.
        DicomDataElement*  dde = findEntry(root->mSibling, group, element, last);
        if (dde!=NULL)    return dde;
        //either we found it or we didn't
        return findEntry(root->mChild, group, element, last);
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    friend std::ostream& operator<< ( std::ostream &out, const DicomReader& d );
};
//======================================================================
#endif

#include "Silf.h"
#include "XmlTraceLog.h"

bool Silf::readGraphite(void *pSilf, size_t lSilf, int numGlyphs, uint32 version)
{
    char *p = (char *)pSilf;
    uint32 *pPasses;
#ifndef DISABLE_TRACING
    XmlTraceLog::get().openElement(ElementSilfSub);
#endif
    if (version >= 0x00030000)
    {
#ifndef DISABLE_TRACING
        XmlTraceLog::get().addAttribute(AttrMajor, swap16(((uint16*) p)[0]));
        XmlTraceLog::get().addAttribute(AttrMinor, swap16(((uint16*) p)[1]));
#endif
        p += 8;
    }
    p += 2;     // maxGlyphID
    p += 4;     // extra ascent/descent
    m_numPasses = *p++;
#ifndef DISABLE_TRACING
    XmlTraceLog::get().addAttribute(AttrNumPasses, m_numPasses);
#endif
    if (m_numPasses < 0) return false;
    m_passes = new Pass[m_numPasses];
    m_sPass = *p++;
#ifndef DISABLE_TRACING
    XmlTraceLog::get().addAttribute(AttrSubPass, m_sPass);
#endif
    if (m_sPass < 0) return false;
    m_pPass = *p++;
#ifndef DISABLE_TRACING
    XmlTraceLog::get().addAttribute(AttrPosPass, m_pPass);
#endif
    if (m_pPass < m_sPass) return false;
    m_jPass = *p++;
#ifndef DISABLE_TRACING
    XmlTraceLog::get().addAttribute(AttrJustPass, m_jPass);
#endif
    if (m_jPass < m_pPass) return false;
    m_bPass = *p++;     // when do we reorder?
#ifndef DISABLE_TRACING
    XmlTraceLog::get().addAttribute(AttrBidiPass, m_bPass);
#endif
    if (m_bPass != 0xFF && (m_bPass < m_jPass || m_bPass > m_numPasses)) return false;
    m_flags = *p++;
    p += 2;     // ignore line end contextuals for now
    p++;        // not sure what to do with attrPseudo
    m_aBreak = *p++;
#ifndef DISABLE_TRACING
    XmlTraceLog::get().addAttribute(AttrBreakWeight, m_aBreak);
#endif
    if (m_aBreak < 0) return false;
#ifndef DISABLE_TRACING
    XmlTraceLog::get().addAttribute(AttrDirectionality, *p);
#endif
    p++;        // we don't do bidi
    p += 2;     // skip reserved stuff
#ifndef DISABLE_TRACING
    XmlTraceLog::get().addAttribute(AttrNumJustLevels, *p);
#endif
    p += *p * 8 + 1;     // ignore justification for now
    m_aLig = read16(p);
#ifndef DISABLE_TRACING
    XmlTraceLog::get().addAttribute(AttrLigComp, *p);
#endif
    if (m_aLig > 127) return false;
    m_aUser = *p++;
#ifndef DISABLE_TRACING
    XmlTraceLog::get().addAttribute(AttrUserDefn, m_aUser);
#endif
    if (m_aUser < 0) return false;
    m_iMaxComp = *p++;
#ifndef DISABLE_TRACING
    XmlTraceLog::get().addAttribute(AttrNumLigComp, m_iMaxComp);
#endif
    if (m_iMaxComp < 0) return false;
    p += 5;     // skip direction and reserved
#ifndef DISABLE_TRACING
    XmlTraceLog::get().addAttribute(AttrNumCritFeatures, *p);
#endif
    p += *p * 2 + 1;        // don't need critical features yet
    p++;        // reserved
    if (p - (char *)pSilf >= static_cast<int32>(lSilf)) return false;
#ifndef DISABLE_TRACING
    XmlTraceLog::get().addAttribute(AttrNumScripts, *p);
#endif
    p += *p * 4 + 1;        // skip scripts
    p += 2;     // skip lbGID
    if (p - (char *)pSilf >= static_cast<int32>(lSilf)) return false;
    pPasses = (uint32 *)p;
    p += 4 * (m_numPasses + 1);
    m_numPseudo = read16(p);
#ifndef DISABLE_TRACING
    XmlTraceLog::get().addAttribute(AttrNumPseudo, m_numPseudo);
#endif
    p += 6;
    m_pseudos = new Pseudo[m_numPseudo];
    for (int i = 0; i < m_numPseudo; i++)
    {
        m_pseudos[i].uid = read32(p);
        m_pseudos[i].gid = read16(p);
#ifndef DISABLE_TRACING
        XmlTraceLog::get().openElement(ElementPseudo);
        XmlTraceLog::get().addAttribute(AttrIndex, i);
        XmlTraceLog::get().addAttribute(AttrGlyphId, m_pseudos[i].uid);
        XmlTraceLog::get().writeUnicode(m_pseudos[i].uid);
        XmlTraceLog::get().closeElement(ElementPseudo);
#endif
    }
    if (p - (char *)pSilf >= static_cast<int32>(lSilf)) return false;

    int clen = readClassMap((void *)p, swap32(*pPasses) - (p - (char *)pSilf), numGlyphs);
    if (clen < 0) return false;
    p += clen;

    for (int i = 0; i < m_numPasses; i++)
    {
        m_passes[i].init(this);
#ifndef DISABLE_TRACING
        XmlTraceLog::get().openElement(ElementPass);
        XmlTraceLog::get().addAttribute(AttrPassId, i);
#endif
        if (!m_passes[i].readPass((char *)pSilf + swap32(pPasses[i]), swap32(pPasses[i + 1]) - swap32(pPasses[i]), numGlyphs + m_numPseudo))
        {
#ifndef DISABLE_TRACING
            XmlTraceLog::get().closeElement(ElementPass);
#endif
            return false;
        }
#ifndef DISABLE_TRACING
        XmlTraceLog::get().closeElement(ElementPass);
#endif
    }
#ifndef DISABLE_TRACING
    XmlTraceLog::get().closeElement(ElementSilfSub);
#endif
    return true;
}

size_t Silf::readClassMap(void *pClass, size_t lClass, int numGlyphs)
{
    char *p = (char *)pClass;
    m_nClass = read16(p);
    m_nLinear = read16(p);
    m_classOffsets = new uint16[m_nClass + 1];
#ifndef DISABLE_TRACING
    XmlTraceLog::get().openElement(ElementClassMap);
    XmlTraceLog::get().addAttribute(AttrNumClasses, m_nClass);
    XmlTraceLog::get().addAttribute(AttrNumLinear, m_nLinear);
#endif

    for (int i = 0; i <= m_nClass; i++)
    {
        m_classOffsets[i] = read16(p) / 2 - (2 + m_nClass + 1);     // uint16[] index
    }

    if (m_classOffsets[0] != 0)
    {
#ifndef DISABLE_TRACING
        XmlTraceLog::get().error("Invalid first Class Offset %d expected %d",
            m_classOffsets[0], m_nLinear);
        XmlTraceLog::get().closeElement(ElementClassMap);
#endif
        return -1;
    }
    if (m_classOffsets[m_nClass] + (2u + m_nClass + 1u) * 2 > lClass)
    {
#ifndef DISABLE_TRACING
        XmlTraceLog::get().error("Invalid Class Offset %d max size %d",
            m_classOffsets[m_nClass], lClass);
        XmlTraceLog::get().closeElement(ElementClassMap);
#endif
        return -1;
    }
    m_classData = new uint16[m_classOffsets[m_nClass]];
    for (int i = 0; i < m_classOffsets[m_nClass]; i++)
        m_classData[i] = read16(p);
#ifndef DISABLE_TRACING
    // TODO this includes extra checking which shouldn't be
    // disabled when tracing is inactive unless it is duplicated elsewhere
    if (XmlTraceLog::get().active())
    {
        bool glyphsOk = true;
        for (int i = 0; i < m_nClass; i++)
        {
            XmlTraceLog::get().openElement(ElementLookupClass);
            XmlTraceLog::get().addAttribute(AttrIndex, i);
            if (i < m_nLinear)
            {
                for (int j = m_classOffsets[i]; j < m_classOffsets[i+1]; j++)
                {
                    XmlTraceLog::get().openElement(ElementLookup);
                    XmlTraceLog::get().addAttribute(AttrGlyphId, m_classData[j]);
                    if (m_classData[j] >= numGlyphs)
                    {
                        XmlTraceLog::get().warning("GlyphId out of range %d",
                            m_classData[j]);
                        glyphsOk = false;
                    }
                    XmlTraceLog::get().closeElement(ElementLookup);
                }
            }
            else
            {
                int offset = m_classOffsets[i];
                uint16 numIds = m_classData[offset];
                XmlTraceLog::get().addAttribute(AttrNum, numIds);
                for (int j = offset + 4; j < m_classOffsets[i+1]; j += 2)
                {
                    XmlTraceLog::get().openElement(ElementLookup);
                    XmlTraceLog::get().addAttribute(AttrGlyphId, m_classData[j]);
                    XmlTraceLog::get().addAttribute(AttrIndex, m_classData[j+1]);
                    if (m_classData[j] >= numGlyphs)
                    {
                        XmlTraceLog::get().warning("GlyphId out of range %d",
                            m_classData[j]);
                        glyphsOk = false;
                    }
                    if (m_classData[j+1] >= numIds)
                    {
                        XmlTraceLog::get().warning("Index out of range %d",
                            m_classData[j+1]);
                        glyphsOk = false;
                    }
                    XmlTraceLog::get().closeElement(ElementLookup);
                }
            }
            XmlTraceLog::get().closeElement(ElementLookupClass);
        }
        XmlTraceLog::get().closeElement(ElementClassMap);
        if (!glyphsOk)
            return -1;
    }
#endif
    return (p - (char *)pClass);
}

uint16 Silf::findPseudo(uint32 uid)
{
    for (int i = 0; i < m_numPseudo; i++)
        if (m_pseudos[i].uid == uid) return m_pseudos[i].gid;
    return 0;
}

uint16 Silf::findClassIndex(uint16 cid, uint16 gid)
{
    if (cid > m_nClass || cid < 0) return -1;

    uint16 loc = m_classOffsets[cid];
    if (cid < m_nLinear)        // output class being used for input, shouldn't happen
    {
        for (int i = loc; i < m_classOffsets[cid + 1]; i++)
            if (m_classData[i] == gid) return i - loc;
    }
    else
    {
        uint16 num = m_classData[loc];
        uint16 search = m_classData[loc + 1] / 2;
        uint16 selector = m_classData[loc + 2];
        uint16 range = m_classData[loc + 3] / 2;

        uint16 curr = loc + 4 + range;

        while (search > 1)
        {
            int test;
            if (curr < loc + 4)
                test = -1;
            else
                test = m_classData[curr] - gid;

            if (test == 0) return m_classData[curr + 1];

            search >>= 1;
            if (test < 0)
                curr += search;
            else
                curr -= search;
        }
    }
    return -1;
}

uint16 Silf::getClassGlyph(uint16 cid, uint16 index)
{
    if (cid > m_nClass || cid < 0) return 0;

    uint16 loc = m_classOffsets[cid];
    if (cid < m_nLinear)
    {
        if (index < m_classOffsets[cid + 1] - index)
            return m_classData[index + loc];
    }
    else        // input class being used for output. Shouldn't happen
    {
        for (int i = loc + 4; i < m_classOffsets[cid + 1]; i += 2)
            if (m_classData[i + 1] == index) return m_classData[i];
    }
    return 0;
}


void Silf::runGraphite(Segment *seg, const LoadedFace *face, VMScratch *vms) const
{
    for (int i = 0; i < m_numPasses; i++)
    {
#ifndef DISABLE_TRACING
	XmlTraceLog::get().openElement(ElementRunPass);
	XmlTraceLog::get().addAttribute(AttrNum, i);
#endif
        // test whether to reorder, prepare for positioning
        m_passes[i].runGraphite(seg, face, vms);
#ifndef DISABLE_TRACING
	XmlTraceLog::get().closeElement(ElementRunPass);
#endif
    }
}

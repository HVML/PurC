/*
 * @file hvml-character-reference.c
 * @author XueShuming
 * @date 2021/09/03
 * @brief The impl for hvml character reference entity.
 *
 * Copyright (C) 2021 FMSoft <https://www.fmsoft.cn>
 *
 * This file is a part of PurC (short for Purring Cat), an HVML interpreter.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */


#include "hvml-character-reference.h"

#include <stddef.h>
#include <stdint.h>

static const char AEligEntityName[] = "AElig";
static const char AEligSemicolonEntityName[] = "AElig;";
static const char AMPEntityName[] = "AMP";
static const char AMPSemicolonEntityName[] = "AMP;";
static const char AacuteEntityName[] = "Aacute";
static const char AacuteSemicolonEntityName[] = "Aacute;";
static const char AbreveSemicolonEntityName[] = "Abreve;";
static const char AcircEntityName[] = "Acirc";
static const char AcircSemicolonEntityName[] = "Acirc;";
static const char AcySemicolonEntityName[] = "Acy;";
static const char AfrSemicolonEntityName[] = "Afr;";
static const char AgraveEntityName[] = "Agrave";
static const char AgraveSemicolonEntityName[] = "Agrave;";
static const char AlphaSemicolonEntityName[] = "Alpha;";
static const char AmacrSemicolonEntityName[] = "Amacr;";
static const char AndSemicolonEntityName[] = "And;";
static const char AogonSemicolonEntityName[] = "Aogon;";
static const char AopfSemicolonEntityName[] = "Aopf;";
static const char ApplyFunctionSemicolonEntityName[] = "ApplyFunction;";
static const char AringEntityName[] = "Aring";
static const char AringSemicolonEntityName[] = "Aring;";
static const char AscrSemicolonEntityName[] = "Ascr;";
static const char AssignSemicolonEntityName[] = "Assign;";
static const char AtildeEntityName[] = "Atilde";
static const char AtildeSemicolonEntityName[] = "Atilde;";
static const char AumlEntityName[] = "Auml";
static const char AumlSemicolonEntityName[] = "Auml;";
static const char BackslashSemicolonEntityName[] = "Backslash;";
static const char BarvSemicolonEntityName[] = "Barv;";
static const char BarwedSemicolonEntityName[] = "Barwed;";
static const char BcySemicolonEntityName[] = "Bcy;";
static const char BecauseSemicolonEntityName[] = "Because;";
static const char BernoullisSemicolonEntityName[] = "Bernoullis;";
static const char BetaSemicolonEntityName[] = "Beta;";
static const char BfrSemicolonEntityName[] = "Bfr;";
static const char BopfSemicolonEntityName[] = "Bopf;";
static const char BreveSemicolonEntityName[] = "Breve;";
static const char BscrSemicolonEntityName[] = "Bscr;";
static const char BumpeqSemicolonEntityName[] = "Bumpeq;";
static const char CHcySemicolonEntityName[] = "CHcy;";
static const char COPYEntityName[] = "COPY";
static const char COPYSemicolonEntityName[] = "COPY;";
static const char CacuteSemicolonEntityName[] = "Cacute;";
static const char CapSemicolonEntityName[] = "Cap;";
static const char CapitalDifferentialDSemicolonEntityName[] = "CapitalDifferentialD;";
static const char CayleysSemicolonEntityName[] = "Cayleys;";
static const char CcaronSemicolonEntityName[] = "Ccaron;";
static const char CcedilEntityName[] = "Ccedil";
static const char CcedilSemicolonEntityName[] = "Ccedil;";
static const char CcircSemicolonEntityName[] = "Ccirc;";
static const char CconintSemicolonEntityName[] = "Cconint;";
static const char CdotSemicolonEntityName[] = "Cdot;";
static const char CedillaSemicolonEntityName[] = "Cedilla;";
static const char CenterDotSemicolonEntityName[] = "CenterDot;";
static const char CfrSemicolonEntityName[] = "Cfr;";
static const char ChiSemicolonEntityName[] = "Chi;";
static const char CircleDotSemicolonEntityName[] = "CircleDot;";
static const char CircleMinusSemicolonEntityName[] = "CircleMinus;";
static const char CirclePlusSemicolonEntityName[] = "CirclePlus;";
static const char CircleTimesSemicolonEntityName[] = "CircleTimes;";
static const char ClockwiseContourIntegralSemicolonEntityName[] = "ClockwiseContourIntegral;";
static const char CloseCurlyDoubleQuoteSemicolonEntityName[] = "CloseCurlyDoubleQuote;";
static const char CloseCurlyQuoteSemicolonEntityName[] = "CloseCurlyQuote;";
static const char ColonSemicolonEntityName[] = "Colon;";
static const char ColoneSemicolonEntityName[] = "Colone;";
static const char CongruentSemicolonEntityName[] = "Congruent;";
static const char ConintSemicolonEntityName[] = "Conint;";
static const char ContourIntegralSemicolonEntityName[] = "ContourIntegral;";
static const char CopfSemicolonEntityName[] = "Copf;";
static const char CoproductSemicolonEntityName[] = "Coproduct;";
static const char CounterClockwiseContourIntegralSemicolonEntityName[] = "CounterClockwiseContourIntegral;";
static const char CrossSemicolonEntityName[] = "Cross;";
static const char CscrSemicolonEntityName[] = "Cscr;";
static const char CupSemicolonEntityName[] = "Cup;";
static const char CupCapSemicolonEntityName[] = "CupCap;";
static const char DDSemicolonEntityName[] = "DD;";
static const char DDotrahdSemicolonEntityName[] = "DDotrahd;";
static const char DJcySemicolonEntityName[] = "DJcy;";
static const char DScySemicolonEntityName[] = "DScy;";
static const char DZcySemicolonEntityName[] = "DZcy;";
static const char DaggerSemicolonEntityName[] = "Dagger;";
static const char DarrSemicolonEntityName[] = "Darr;";
static const char DashvSemicolonEntityName[] = "Dashv;";
static const char DcaronSemicolonEntityName[] = "Dcaron;";
static const char DcySemicolonEntityName[] = "Dcy;";
static const char DelSemicolonEntityName[] = "Del;";
static const char DeltaSemicolonEntityName[] = "Delta;";
static const char DfrSemicolonEntityName[] = "Dfr;";
static const char DiacriticalAcuteSemicolonEntityName[] = "DiacriticalAcute;";
static const char DiacriticalDotSemicolonEntityName[] = "DiacriticalDot;";
static const char DiacriticalDoubleAcuteSemicolonEntityName[] = "DiacriticalDoubleAcute;";
static const char DiacriticalGraveSemicolonEntityName[] = "DiacriticalGrave;";
static const char DiacriticalTildeSemicolonEntityName[] = "DiacriticalTilde;";
static const char DiamondSemicolonEntityName[] = "Diamond;";
static const char DifferentialDSemicolonEntityName[] = "DifferentialD;";
static const char DopfSemicolonEntityName[] = "Dopf;";
static const char DotSemicolonEntityName[] = "Dot;";
static const char DotDotSemicolonEntityName[] = "DotDot;";
static const char DotEqualSemicolonEntityName[] = "DotEqual;";
static const char DoubleContourIntegralSemicolonEntityName[] = "DoubleContourIntegral;";
static const char DoubleDotSemicolonEntityName[] = "DoubleDot;";
static const char DoubleDownArrowSemicolonEntityName[] = "DoubleDownArrow;";
static const char DoubleLeftArrowSemicolonEntityName[] = "DoubleLeftArrow;";
static const char DoubleLeftRightArrowSemicolonEntityName[] = "DoubleLeftRightArrow;";
static const char DoubleLeftTeeSemicolonEntityName[] = "DoubleLeftTee;";
static const char DoubleLongLeftArrowSemicolonEntityName[] = "DoubleLongLeftArrow;";
static const char DoubleLongLeftRightArrowSemicolonEntityName[] = "DoubleLongLeftRightArrow;";
static const char DoubleLongRightArrowSemicolonEntityName[] = "DoubleLongRightArrow;";
static const char DoubleRightArrowSemicolonEntityName[] = "DoubleRightArrow;";
static const char DoubleRightTeeSemicolonEntityName[] = "DoubleRightTee;";
static const char DoubleUpArrowSemicolonEntityName[] = "DoubleUpArrow;";
static const char DoubleUpDownArrowSemicolonEntityName[] = "DoubleUpDownArrow;";
static const char DoubleVerticalBarSemicolonEntityName[] = "DoubleVerticalBar;";
static const char DownArrowSemicolonEntityName[] = "DownArrow;";
static const char DownArrowBarSemicolonEntityName[] = "DownArrowBar;";
static const char DownArrowUpArrowSemicolonEntityName[] = "DownArrowUpArrow;";
static const char DownBreveSemicolonEntityName[] = "DownBreve;";
static const char DownLeftRightVectorSemicolonEntityName[] = "DownLeftRightVector;";
static const char DownLeftTeeVectorSemicolonEntityName[] = "DownLeftTeeVector;";
static const char DownLeftVectorSemicolonEntityName[] = "DownLeftVector;";
static const char DownLeftVectorBarSemicolonEntityName[] = "DownLeftVectorBar;";
static const char DownRightTeeVectorSemicolonEntityName[] = "DownRightTeeVector;";
static const char DownRightVectorSemicolonEntityName[] = "DownRightVector;";
static const char DownRightVectorBarSemicolonEntityName[] = "DownRightVectorBar;";
static const char DownTeeSemicolonEntityName[] = "DownTee;";
static const char DownTeeArrowSemicolonEntityName[] = "DownTeeArrow;";
static const char DownarrowSemicolonEntityName[] = "Downarrow;";
static const char DscrSemicolonEntityName[] = "Dscr;";
static const char DstrokSemicolonEntityName[] = "Dstrok;";
static const char ENGSemicolonEntityName[] = "ENG;";
static const char ETHEntityName[] = "ETH";
static const char ETHSemicolonEntityName[] = "ETH;";
static const char EacuteEntityName[] = "Eacute";
static const char EacuteSemicolonEntityName[] = "Eacute;";
static const char EcaronSemicolonEntityName[] = "Ecaron;";
static const char EcircEntityName[] = "Ecirc";
static const char EcircSemicolonEntityName[] = "Ecirc;";
static const char EcySemicolonEntityName[] = "Ecy;";
static const char EdotSemicolonEntityName[] = "Edot;";
static const char EfrSemicolonEntityName[] = "Efr;";
static const char EgraveEntityName[] = "Egrave";
static const char EgraveSemicolonEntityName[] = "Egrave;";
static const char ElementSemicolonEntityName[] = "Element;";
static const char EmacrSemicolonEntityName[] = "Emacr;";
static const char EmptySmallSquareSemicolonEntityName[] = "EmptySmallSquare;";
static const char EmptyVerySmallSquareSemicolonEntityName[] = "EmptyVerySmallSquare;";
static const char EogonSemicolonEntityName[] = "Eogon;";
static const char EopfSemicolonEntityName[] = "Eopf;";
static const char EpsilonSemicolonEntityName[] = "Epsilon;";
static const char EqualSemicolonEntityName[] = "Equal;";
static const char EqualTildeSemicolonEntityName[] = "EqualTilde;";
static const char EquilibriumSemicolonEntityName[] = "Equilibrium;";
static const char EscrSemicolonEntityName[] = "Escr;";
static const char EsimSemicolonEntityName[] = "Esim;";
static const char EtaSemicolonEntityName[] = "Eta;";
static const char EumlEntityName[] = "Euml";
static const char EumlSemicolonEntityName[] = "Euml;";
static const char ExistsSemicolonEntityName[] = "Exists;";
static const char ExponentialESemicolonEntityName[] = "ExponentialE;";
static const char FcySemicolonEntityName[] = "Fcy;";
static const char FfrSemicolonEntityName[] = "Ffr;";
static const char FilledSmallSquareSemicolonEntityName[] = "FilledSmallSquare;";
static const char FilledVerySmallSquareSemicolonEntityName[] = "FilledVerySmallSquare;";
static const char FopfSemicolonEntityName[] = "Fopf;";
static const char ForAllSemicolonEntityName[] = "ForAll;";
static const char FouriertrfSemicolonEntityName[] = "Fouriertrf;";
static const char FscrSemicolonEntityName[] = "Fscr;";
static const char GJcySemicolonEntityName[] = "GJcy;";
static const char GTEntityName[] = "GT";
static const char GTSemicolonEntityName[] = "GT;";
static const char GammaSemicolonEntityName[] = "Gamma;";
static const char GammadSemicolonEntityName[] = "Gammad;";
static const char GbreveSemicolonEntityName[] = "Gbreve;";
static const char GcedilSemicolonEntityName[] = "Gcedil;";
static const char GcircSemicolonEntityName[] = "Gcirc;";
static const char GcySemicolonEntityName[] = "Gcy;";
static const char GdotSemicolonEntityName[] = "Gdot;";
static const char GfrSemicolonEntityName[] = "Gfr;";
static const char GgSemicolonEntityName[] = "Gg;";
static const char GopfSemicolonEntityName[] = "Gopf;";
static const char GreaterEqualSemicolonEntityName[] = "GreaterEqual;";
static const char GreaterEqualLessSemicolonEntityName[] = "GreaterEqualLess;";
static const char GreaterFullEqualSemicolonEntityName[] = "GreaterFullEqual;";
static const char GreaterGreaterSemicolonEntityName[] = "GreaterGreater;";
static const char GreaterLessSemicolonEntityName[] = "GreaterLess;";
static const char GreaterSlantEqualSemicolonEntityName[] = "GreaterSlantEqual;";
static const char GreaterTildeSemicolonEntityName[] = "GreaterTilde;";
static const char GscrSemicolonEntityName[] = "Gscr;";
static const char GtSemicolonEntityName[] = "Gt;";
static const char HARDcySemicolonEntityName[] = "HARDcy;";
static const char HacekSemicolonEntityName[] = "Hacek;";
static const char HatSemicolonEntityName[] = "Hat;";
static const char HcircSemicolonEntityName[] = "Hcirc;";
static const char HfrSemicolonEntityName[] = "Hfr;";
static const char HilbertSpaceSemicolonEntityName[] = "HilbertSpace;";
static const char HopfSemicolonEntityName[] = "Hopf;";
static const char HorizontalLineSemicolonEntityName[] = "HorizontalLine;";
static const char HscrSemicolonEntityName[] = "Hscr;";
static const char HstrokSemicolonEntityName[] = "Hstrok;";
static const char HumpDownHumpSemicolonEntityName[] = "HumpDownHump;";
static const char HumpEqualSemicolonEntityName[] = "HumpEqual;";
static const char IEcySemicolonEntityName[] = "IEcy;";
static const char IJligSemicolonEntityName[] = "IJlig;";
static const char IOcySemicolonEntityName[] = "IOcy;";
static const char IacuteEntityName[] = "Iacute";
static const char IacuteSemicolonEntityName[] = "Iacute;";
static const char IcircEntityName[] = "Icirc";
static const char IcircSemicolonEntityName[] = "Icirc;";
static const char IcySemicolonEntityName[] = "Icy;";
static const char IdotSemicolonEntityName[] = "Idot;";
static const char IfrSemicolonEntityName[] = "Ifr;";
static const char IgraveEntityName[] = "Igrave";
static const char IgraveSemicolonEntityName[] = "Igrave;";
static const char ImSemicolonEntityName[] = "Im;";
static const char ImacrSemicolonEntityName[] = "Imacr;";
static const char ImaginaryISemicolonEntityName[] = "ImaginaryI;";
static const char ImpliesSemicolonEntityName[] = "Implies;";
static const char IntSemicolonEntityName[] = "Int;";
static const char IntegralSemicolonEntityName[] = "Integral;";
static const char IntersectionSemicolonEntityName[] = "Intersection;";
static const char InvisibleCommaSemicolonEntityName[] = "InvisibleComma;";
static const char InvisibleTimesSemicolonEntityName[] = "InvisibleTimes;";
static const char IogonSemicolonEntityName[] = "Iogon;";
static const char IopfSemicolonEntityName[] = "Iopf;";
static const char IotaSemicolonEntityName[] = "Iota;";
static const char IscrSemicolonEntityName[] = "Iscr;";
static const char ItildeSemicolonEntityName[] = "Itilde;";
static const char IukcySemicolonEntityName[] = "Iukcy;";
static const char IumlEntityName[] = "Iuml";
static const char IumlSemicolonEntityName[] = "Iuml;";
static const char JcircSemicolonEntityName[] = "Jcirc;";
static const char JcySemicolonEntityName[] = "Jcy;";
static const char JfrSemicolonEntityName[] = "Jfr;";
static const char JopfSemicolonEntityName[] = "Jopf;";
static const char JscrSemicolonEntityName[] = "Jscr;";
static const char JsercySemicolonEntityName[] = "Jsercy;";
static const char JukcySemicolonEntityName[] = "Jukcy;";
static const char KHcySemicolonEntityName[] = "KHcy;";
static const char KJcySemicolonEntityName[] = "KJcy;";
static const char KappaSemicolonEntityName[] = "Kappa;";
static const char KcedilSemicolonEntityName[] = "Kcedil;";
static const char KcySemicolonEntityName[] = "Kcy;";
static const char KfrSemicolonEntityName[] = "Kfr;";
static const char KopfSemicolonEntityName[] = "Kopf;";
static const char KscrSemicolonEntityName[] = "Kscr;";
static const char LJcySemicolonEntityName[] = "LJcy;";
static const char LTEntityName[] = "LT";
static const char LTSemicolonEntityName[] = "LT;";
static const char LacuteSemicolonEntityName[] = "Lacute;";
static const char LambdaSemicolonEntityName[] = "Lambda;";
static const char LangSemicolonEntityName[] = "Lang;";
static const char LaplacetrfSemicolonEntityName[] = "Laplacetrf;";
static const char LarrSemicolonEntityName[] = "Larr;";
static const char LcaronSemicolonEntityName[] = "Lcaron;";
static const char LcedilSemicolonEntityName[] = "Lcedil;";
static const char LcySemicolonEntityName[] = "Lcy;";
static const char LeftAngleBracketSemicolonEntityName[] = "LeftAngleBracket;";
static const char LeftArrowSemicolonEntityName[] = "LeftArrow;";
static const char LeftArrowBarSemicolonEntityName[] = "LeftArrowBar;";
static const char LeftArrowRightArrowSemicolonEntityName[] = "LeftArrowRightArrow;";
static const char LeftCeilingSemicolonEntityName[] = "LeftCeiling;";
static const char LeftDoubleBracketSemicolonEntityName[] = "LeftDoubleBracket;";
static const char LeftDownTeeVectorSemicolonEntityName[] = "LeftDownTeeVector;";
static const char LeftDownVectorSemicolonEntityName[] = "LeftDownVector;";
static const char LeftDownVectorBarSemicolonEntityName[] = "LeftDownVectorBar;";
static const char LeftFloorSemicolonEntityName[] = "LeftFloor;";
static const char LeftRightArrowSemicolonEntityName[] = "LeftRightArrow;";
static const char LeftRightVectorSemicolonEntityName[] = "LeftRightVector;";
static const char LeftTeeSemicolonEntityName[] = "LeftTee;";
static const char LeftTeeArrowSemicolonEntityName[] = "LeftTeeArrow;";
static const char LeftTeeVectorSemicolonEntityName[] = "LeftTeeVector;";
static const char LeftTriangleSemicolonEntityName[] = "LeftTriangle;";
static const char LeftTriangleBarSemicolonEntityName[] = "LeftTriangleBar;";
static const char LeftTriangleEqualSemicolonEntityName[] = "LeftTriangleEqual;";
static const char LeftUpDownVectorSemicolonEntityName[] = "LeftUpDownVector;";
static const char LeftUpTeeVectorSemicolonEntityName[] = "LeftUpTeeVector;";
static const char LeftUpVectorSemicolonEntityName[] = "LeftUpVector;";
static const char LeftUpVectorBarSemicolonEntityName[] = "LeftUpVectorBar;";
static const char LeftVectorSemicolonEntityName[] = "LeftVector;";
static const char LeftVectorBarSemicolonEntityName[] = "LeftVectorBar;";
static const char LeftarrowSemicolonEntityName[] = "Leftarrow;";
static const char LeftrightarrowSemicolonEntityName[] = "Leftrightarrow;";
static const char LessEqualGreaterSemicolonEntityName[] = "LessEqualGreater;";
static const char LessFullEqualSemicolonEntityName[] = "LessFullEqual;";
static const char LessGreaterSemicolonEntityName[] = "LessGreater;";
static const char LessLessSemicolonEntityName[] = "LessLess;";
static const char LessSlantEqualSemicolonEntityName[] = "LessSlantEqual;";
static const char LessTildeSemicolonEntityName[] = "LessTilde;";
static const char LfrSemicolonEntityName[] = "Lfr;";
static const char LlSemicolonEntityName[] = "Ll;";
static const char LleftarrowSemicolonEntityName[] = "Lleftarrow;";
static const char LmidotSemicolonEntityName[] = "Lmidot;";
static const char LongLeftArrowSemicolonEntityName[] = "LongLeftArrow;";
static const char LongLeftRightArrowSemicolonEntityName[] = "LongLeftRightArrow;";
static const char LongRightArrowSemicolonEntityName[] = "LongRightArrow;";
static const char LongleftarrowSemicolonEntityName[] = "Longleftarrow;";
static const char LongleftrightarrowSemicolonEntityName[] = "Longleftrightarrow;";
static const char LongrightarrowSemicolonEntityName[] = "Longrightarrow;";
static const char LopfSemicolonEntityName[] = "Lopf;";
static const char LowerLeftArrowSemicolonEntityName[] = "LowerLeftArrow;";
static const char LowerRightArrowSemicolonEntityName[] = "LowerRightArrow;";
static const char LscrSemicolonEntityName[] = "Lscr;";
static const char LshSemicolonEntityName[] = "Lsh;";
static const char LstrokSemicolonEntityName[] = "Lstrok;";
static const char LtSemicolonEntityName[] = "Lt;";
static const char MapSemicolonEntityName[] = "Map;";
static const char McySemicolonEntityName[] = "Mcy;";
static const char MediumSpaceSemicolonEntityName[] = "MediumSpace;";
static const char MellintrfSemicolonEntityName[] = "Mellintrf;";
static const char MfrSemicolonEntityName[] = "Mfr;";
static const char MinusPlusSemicolonEntityName[] = "MinusPlus;";
static const char MopfSemicolonEntityName[] = "Mopf;";
static const char MscrSemicolonEntityName[] = "Mscr;";
static const char MuSemicolonEntityName[] = "Mu;";
static const char NJcySemicolonEntityName[] = "NJcy;";
static const char NacuteSemicolonEntityName[] = "Nacute;";
static const char NcaronSemicolonEntityName[] = "Ncaron;";
static const char NcedilSemicolonEntityName[] = "Ncedil;";
static const char NcySemicolonEntityName[] = "Ncy;";
static const char NegativeMediumSpaceSemicolonEntityName[] = "NegativeMediumSpace;";
static const char NegativeThickSpaceSemicolonEntityName[] = "NegativeThickSpace;";
static const char NegativeThinSpaceSemicolonEntityName[] = "NegativeThinSpace;";
static const char NegativeVeryThinSpaceSemicolonEntityName[] = "NegativeVeryThinSpace;";
static const char NestedGreaterGreaterSemicolonEntityName[] = "NestedGreaterGreater;";
static const char NestedLessLessSemicolonEntityName[] = "NestedLessLess;";
static const char NewLineSemicolonEntityName[] = "NewLine;";
static const char NfrSemicolonEntityName[] = "Nfr;";
static const char NoBreakSemicolonEntityName[] = "NoBreak;";
static const char NonBreakingSpaceSemicolonEntityName[] = "NonBreakingSpace;";
static const char NopfSemicolonEntityName[] = "Nopf;";
static const char NotSemicolonEntityName[] = "Not;";
static const char NotCongruentSemicolonEntityName[] = "NotCongruent;";
static const char NotCupCapSemicolonEntityName[] = "NotCupCap;";
static const char NotDoubleVerticalBarSemicolonEntityName[] = "NotDoubleVerticalBar;";
static const char NotElementSemicolonEntityName[] = "NotElement;";
static const char NotEqualSemicolonEntityName[] = "NotEqual;";
static const char NotEqualTildeSemicolonEntityName[] = "NotEqualTilde;";
static const char NotExistsSemicolonEntityName[] = "NotExists;";
static const char NotGreaterSemicolonEntityName[] = "NotGreater;";
static const char NotGreaterEqualSemicolonEntityName[] = "NotGreaterEqual;";
static const char NotGreaterFullEqualSemicolonEntityName[] = "NotGreaterFullEqual;";
static const char NotGreaterGreaterSemicolonEntityName[] = "NotGreaterGreater;";
static const char NotGreaterLessSemicolonEntityName[] = "NotGreaterLess;";
static const char NotGreaterSlantEqualSemicolonEntityName[] = "NotGreaterSlantEqual;";
static const char NotGreaterTildeSemicolonEntityName[] = "NotGreaterTilde;";
static const char NotHumpDownHumpSemicolonEntityName[] = "NotHumpDownHump;";
static const char NotHumpEqualSemicolonEntityName[] = "NotHumpEqual;";
static const char NotLeftTriangleSemicolonEntityName[] = "NotLeftTriangle;";
static const char NotLeftTriangleBarSemicolonEntityName[] = "NotLeftTriangleBar;";
static const char NotLeftTriangleEqualSemicolonEntityName[] = "NotLeftTriangleEqual;";
static const char NotLessSemicolonEntityName[] = "NotLess;";
static const char NotLessEqualSemicolonEntityName[] = "NotLessEqual;";
static const char NotLessGreaterSemicolonEntityName[] = "NotLessGreater;";
static const char NotLessLessSemicolonEntityName[] = "NotLessLess;";
static const char NotLessSlantEqualSemicolonEntityName[] = "NotLessSlantEqual;";
static const char NotLessTildeSemicolonEntityName[] = "NotLessTilde;";
static const char NotNestedGreaterGreaterSemicolonEntityName[] = "NotNestedGreaterGreater;";
static const char NotNestedLessLessSemicolonEntityName[] = "NotNestedLessLess;";
static const char NotPrecedesSemicolonEntityName[] = "NotPrecedes;";
static const char NotPrecedesEqualSemicolonEntityName[] = "NotPrecedesEqual;";
static const char NotPrecedesSlantEqualSemicolonEntityName[] = "NotPrecedesSlantEqual;";
static const char NotReverseElementSemicolonEntityName[] = "NotReverseElement;";
static const char NotRightTriangleSemicolonEntityName[] = "NotRightTriangle;";
static const char NotRightTriangleBarSemicolonEntityName[] = "NotRightTriangleBar;";
static const char NotRightTriangleEqualSemicolonEntityName[] = "NotRightTriangleEqual;";
static const char NotSquareSubsetSemicolonEntityName[] = "NotSquareSubset;";
static const char NotSquareSubsetEqualSemicolonEntityName[] = "NotSquareSubsetEqual;";
static const char NotSquareSupersetSemicolonEntityName[] = "NotSquareSuperset;";
static const char NotSquareSupersetEqualSemicolonEntityName[] = "NotSquareSupersetEqual;";
static const char NotSubsetSemicolonEntityName[] = "NotSubset;";
static const char NotSubsetEqualSemicolonEntityName[] = "NotSubsetEqual;";
static const char NotSucceedsSemicolonEntityName[] = "NotSucceeds;";
static const char NotSucceedsEqualSemicolonEntityName[] = "NotSucceedsEqual;";
static const char NotSucceedsSlantEqualSemicolonEntityName[] = "NotSucceedsSlantEqual;";
static const char NotSucceedsTildeSemicolonEntityName[] = "NotSucceedsTilde;";
static const char NotSupersetSemicolonEntityName[] = "NotSuperset;";
static const char NotSupersetEqualSemicolonEntityName[] = "NotSupersetEqual;";
static const char NotTildeSemicolonEntityName[] = "NotTilde;";
static const char NotTildeEqualSemicolonEntityName[] = "NotTildeEqual;";
static const char NotTildeFullEqualSemicolonEntityName[] = "NotTildeFullEqual;";
static const char NotTildeTildeSemicolonEntityName[] = "NotTildeTilde;";
static const char NotVerticalBarSemicolonEntityName[] = "NotVerticalBar;";
static const char NscrSemicolonEntityName[] = "Nscr;";
static const char NtildeEntityName[] = "Ntilde";
static const char NtildeSemicolonEntityName[] = "Ntilde;";
static const char NuSemicolonEntityName[] = "Nu;";
static const char OEligSemicolonEntityName[] = "OElig;";
static const char OacuteEntityName[] = "Oacute";
static const char OacuteSemicolonEntityName[] = "Oacute;";
static const char OcircEntityName[] = "Ocirc";
static const char OcircSemicolonEntityName[] = "Ocirc;";
static const char OcySemicolonEntityName[] = "Ocy;";
static const char OdblacSemicolonEntityName[] = "Odblac;";
static const char OfrSemicolonEntityName[] = "Ofr;";
static const char OgraveEntityName[] = "Ograve";
static const char OgraveSemicolonEntityName[] = "Ograve;";
static const char OmacrSemicolonEntityName[] = "Omacr;";
static const char OmegaSemicolonEntityName[] = "Omega;";
static const char OmicronSemicolonEntityName[] = "Omicron;";
static const char OopfSemicolonEntityName[] = "Oopf;";
static const char OpenCurlyDoubleQuoteSemicolonEntityName[] = "OpenCurlyDoubleQuote;";
static const char OpenCurlyQuoteSemicolonEntityName[] = "OpenCurlyQuote;";
static const char OrSemicolonEntityName[] = "Or;";
static const char OscrSemicolonEntityName[] = "Oscr;";
static const char OslashEntityName[] = "Oslash";
static const char OslashSemicolonEntityName[] = "Oslash;";
static const char OtildeEntityName[] = "Otilde";
static const char OtildeSemicolonEntityName[] = "Otilde;";
static const char OtimesSemicolonEntityName[] = "Otimes;";
static const char OumlEntityName[] = "Ouml";
static const char OumlSemicolonEntityName[] = "Ouml;";
static const char OverBarSemicolonEntityName[] = "OverBar;";
static const char OverBraceSemicolonEntityName[] = "OverBrace;";
static const char OverBracketSemicolonEntityName[] = "OverBracket;";
static const char OverParenthesisSemicolonEntityName[] = "OverParenthesis;";
static const char PartialDSemicolonEntityName[] = "PartialD;";
static const char PcySemicolonEntityName[] = "Pcy;";
static const char PfrSemicolonEntityName[] = "Pfr;";
static const char PhiSemicolonEntityName[] = "Phi;";
static const char PiSemicolonEntityName[] = "Pi;";
static const char PlusMinusSemicolonEntityName[] = "PlusMinus;";
static const char PoincareplaneSemicolonEntityName[] = "Poincareplane;";
static const char PopfSemicolonEntityName[] = "Popf;";
static const char PrSemicolonEntityName[] = "Pr;";
static const char PrecedesSemicolonEntityName[] = "Precedes;";
static const char PrecedesEqualSemicolonEntityName[] = "PrecedesEqual;";
static const char PrecedesSlantEqualSemicolonEntityName[] = "PrecedesSlantEqual;";
static const char PrecedesTildeSemicolonEntityName[] = "PrecedesTilde;";
static const char PrimeSemicolonEntityName[] = "Prime;";
static const char ProductSemicolonEntityName[] = "Product;";
static const char ProportionSemicolonEntityName[] = "Proportion;";
static const char ProportionalSemicolonEntityName[] = "Proportional;";
static const char PscrSemicolonEntityName[] = "Pscr;";
static const char PsiSemicolonEntityName[] = "Psi;";
static const char QUOTEntityName[] = "QUOT";
static const char QUOTSemicolonEntityName[] = "QUOT;";
static const char QfrSemicolonEntityName[] = "Qfr;";
static const char QopfSemicolonEntityName[] = "Qopf;";
static const char QscrSemicolonEntityName[] = "Qscr;";
static const char RBarrSemicolonEntityName[] = "RBarr;";
static const char REGEntityName[] = "REG";
static const char REGSemicolonEntityName[] = "REG;";
static const char RacuteSemicolonEntityName[] = "Racute;";
static const char RangSemicolonEntityName[] = "Rang;";
static const char RarrSemicolonEntityName[] = "Rarr;";
static const char RarrtlSemicolonEntityName[] = "Rarrtl;";
static const char RcaronSemicolonEntityName[] = "Rcaron;";
static const char RcedilSemicolonEntityName[] = "Rcedil;";
static const char RcySemicolonEntityName[] = "Rcy;";
static const char ReSemicolonEntityName[] = "Re;";
static const char ReverseElementSemicolonEntityName[] = "ReverseElement;";
static const char ReverseEquilibriumSemicolonEntityName[] = "ReverseEquilibrium;";
static const char ReverseUpEquilibriumSemicolonEntityName[] = "ReverseUpEquilibrium;";
static const char RfrSemicolonEntityName[] = "Rfr;";
static const char RhoSemicolonEntityName[] = "Rho;";
static const char RightAngleBracketSemicolonEntityName[] = "RightAngleBracket;";
static const char RightArrowSemicolonEntityName[] = "RightArrow;";
static const char RightArrowBarSemicolonEntityName[] = "RightArrowBar;";
static const char RightArrowLeftArrowSemicolonEntityName[] = "RightArrowLeftArrow;";
static const char RightCeilingSemicolonEntityName[] = "RightCeiling;";
static const char RightDoubleBracketSemicolonEntityName[] = "RightDoubleBracket;";
static const char RightDownTeeVectorSemicolonEntityName[] = "RightDownTeeVector;";
static const char RightDownVectorSemicolonEntityName[] = "RightDownVector;";
static const char RightDownVectorBarSemicolonEntityName[] = "RightDownVectorBar;";
static const char RightFloorSemicolonEntityName[] = "RightFloor;";
static const char RightTeeSemicolonEntityName[] = "RightTee;";
static const char RightTeeArrowSemicolonEntityName[] = "RightTeeArrow;";
static const char RightTeeVectorSemicolonEntityName[] = "RightTeeVector;";
static const char RightTriangleSemicolonEntityName[] = "RightTriangle;";
static const char RightTriangleBarSemicolonEntityName[] = "RightTriangleBar;";
static const char RightTriangleEqualSemicolonEntityName[] = "RightTriangleEqual;";
static const char RightUpDownVectorSemicolonEntityName[] = "RightUpDownVector;";
static const char RightUpTeeVectorSemicolonEntityName[] = "RightUpTeeVector;";
static const char RightUpVectorSemicolonEntityName[] = "RightUpVector;";
static const char RightUpVectorBarSemicolonEntityName[] = "RightUpVectorBar;";
static const char RightVectorSemicolonEntityName[] = "RightVector;";
static const char RightVectorBarSemicolonEntityName[] = "RightVectorBar;";
static const char RightarrowSemicolonEntityName[] = "Rightarrow;";
static const char RopfSemicolonEntityName[] = "Ropf;";
static const char RoundImpliesSemicolonEntityName[] = "RoundImplies;";
static const char RrightarrowSemicolonEntityName[] = "Rrightarrow;";
static const char RscrSemicolonEntityName[] = "Rscr;";
static const char RshSemicolonEntityName[] = "Rsh;";
static const char RuleDelayedSemicolonEntityName[] = "RuleDelayed;";
static const char SHCHcySemicolonEntityName[] = "SHCHcy;";
static const char SHcySemicolonEntityName[] = "SHcy;";
static const char SOFTcySemicolonEntityName[] = "SOFTcy;";
static const char SacuteSemicolonEntityName[] = "Sacute;";
static const char ScSemicolonEntityName[] = "Sc;";
static const char ScaronSemicolonEntityName[] = "Scaron;";
static const char ScedilSemicolonEntityName[] = "Scedil;";
static const char ScircSemicolonEntityName[] = "Scirc;";
static const char ScySemicolonEntityName[] = "Scy;";
static const char SfrSemicolonEntityName[] = "Sfr;";
static const char ShortDownArrowSemicolonEntityName[] = "ShortDownArrow;";
static const char ShortLeftArrowSemicolonEntityName[] = "ShortLeftArrow;";
static const char ShortRightArrowSemicolonEntityName[] = "ShortRightArrow;";
static const char ShortUpArrowSemicolonEntityName[] = "ShortUpArrow;";
static const char SigmaSemicolonEntityName[] = "Sigma;";
static const char SmallCircleSemicolonEntityName[] = "SmallCircle;";
static const char SopfSemicolonEntityName[] = "Sopf;";
static const char SqrtSemicolonEntityName[] = "Sqrt;";
static const char SquareSemicolonEntityName[] = "Square;";
static const char SquareIntersectionSemicolonEntityName[] = "SquareIntersection;";
static const char SquareSubsetSemicolonEntityName[] = "SquareSubset;";
static const char SquareSubsetEqualSemicolonEntityName[] = "SquareSubsetEqual;";
static const char SquareSupersetSemicolonEntityName[] = "SquareSuperset;";
static const char SquareSupersetEqualSemicolonEntityName[] = "SquareSupersetEqual;";
static const char SquareUnionSemicolonEntityName[] = "SquareUnion;";
static const char SscrSemicolonEntityName[] = "Sscr;";
static const char StarSemicolonEntityName[] = "Star;";
static const char SubSemicolonEntityName[] = "Sub;";
static const char SubsetSemicolonEntityName[] = "Subset;";
static const char SubsetEqualSemicolonEntityName[] = "SubsetEqual;";
static const char SucceedsSemicolonEntityName[] = "Succeeds;";
static const char SucceedsEqualSemicolonEntityName[] = "SucceedsEqual;";
static const char SucceedsSlantEqualSemicolonEntityName[] = "SucceedsSlantEqual;";
static const char SucceedsTildeSemicolonEntityName[] = "SucceedsTilde;";
static const char SuchThatSemicolonEntityName[] = "SuchThat;";
static const char SumSemicolonEntityName[] = "Sum;";
static const char SupSemicolonEntityName[] = "Sup;";
static const char SupersetSemicolonEntityName[] = "Superset;";
static const char SupersetEqualSemicolonEntityName[] = "SupersetEqual;";
static const char SupsetSemicolonEntityName[] = "Supset;";
static const char THORNEntityName[] = "THORN";
static const char THORNSemicolonEntityName[] = "THORN;";
static const char TRADESemicolonEntityName[] = "TRADE;";
static const char TSHcySemicolonEntityName[] = "TSHcy;";
static const char TScySemicolonEntityName[] = "TScy;";
static const char TabSemicolonEntityName[] = "Tab;";
static const char TauSemicolonEntityName[] = "Tau;";
static const char TcaronSemicolonEntityName[] = "Tcaron;";
static const char TcedilSemicolonEntityName[] = "Tcedil;";
static const char TcySemicolonEntityName[] = "Tcy;";
static const char TfrSemicolonEntityName[] = "Tfr;";
static const char ThereforeSemicolonEntityName[] = "Therefore;";
static const char ThetaSemicolonEntityName[] = "Theta;";
static const char ThickSpaceSemicolonEntityName[] = "ThickSpace;";
static const char ThinSpaceSemicolonEntityName[] = "ThinSpace;";
static const char TildeSemicolonEntityName[] = "Tilde;";
static const char TildeEqualSemicolonEntityName[] = "TildeEqual;";
static const char TildeFullEqualSemicolonEntityName[] = "TildeFullEqual;";
static const char TildeTildeSemicolonEntityName[] = "TildeTilde;";
static const char TopfSemicolonEntityName[] = "Topf;";
static const char TripleDotSemicolonEntityName[] = "TripleDot;";
static const char TscrSemicolonEntityName[] = "Tscr;";
static const char TstrokSemicolonEntityName[] = "Tstrok;";
static const char UacuteEntityName[] = "Uacute";
static const char UacuteSemicolonEntityName[] = "Uacute;";
static const char UarrSemicolonEntityName[] = "Uarr;";
static const char UarrocirSemicolonEntityName[] = "Uarrocir;";
static const char UbrcySemicolonEntityName[] = "Ubrcy;";
static const char UbreveSemicolonEntityName[] = "Ubreve;";
static const char UcircEntityName[] = "Ucirc";
static const char UcircSemicolonEntityName[] = "Ucirc;";
static const char UcySemicolonEntityName[] = "Ucy;";
static const char UdblacSemicolonEntityName[] = "Udblac;";
static const char UfrSemicolonEntityName[] = "Ufr;";
static const char UgraveEntityName[] = "Ugrave";
static const char UgraveSemicolonEntityName[] = "Ugrave;";
static const char UmacrSemicolonEntityName[] = "Umacr;";
static const char UnderBarSemicolonEntityName[] = "UnderBar;";
static const char UnderBraceSemicolonEntityName[] = "UnderBrace;";
static const char UnderBracketSemicolonEntityName[] = "UnderBracket;";
static const char UnderParenthesisSemicolonEntityName[] = "UnderParenthesis;";
static const char UnionSemicolonEntityName[] = "Union;";
static const char UnionPlusSemicolonEntityName[] = "UnionPlus;";
static const char UogonSemicolonEntityName[] = "Uogon;";
static const char UopfSemicolonEntityName[] = "Uopf;";
static const char UpArrowSemicolonEntityName[] = "UpArrow;";
static const char UpArrowBarSemicolonEntityName[] = "UpArrowBar;";
static const char UpArrowDownArrowSemicolonEntityName[] = "UpArrowDownArrow;";
static const char UpDownArrowSemicolonEntityName[] = "UpDownArrow;";
static const char UpEquilibriumSemicolonEntityName[] = "UpEquilibrium;";
static const char UpTeeSemicolonEntityName[] = "UpTee;";
static const char UpTeeArrowSemicolonEntityName[] = "UpTeeArrow;";
static const char UparrowSemicolonEntityName[] = "Uparrow;";
static const char UpdownarrowSemicolonEntityName[] = "Updownarrow;";
static const char UpperLeftArrowSemicolonEntityName[] = "UpperLeftArrow;";
static const char UpperRightArrowSemicolonEntityName[] = "UpperRightArrow;";
static const char UpsiSemicolonEntityName[] = "Upsi;";
static const char UpsilonSemicolonEntityName[] = "Upsilon;";
static const char UringSemicolonEntityName[] = "Uring;";
static const char UscrSemicolonEntityName[] = "Uscr;";
static const char UtildeSemicolonEntityName[] = "Utilde;";
static const char UumlEntityName[] = "Uuml";
static const char UumlSemicolonEntityName[] = "Uuml;";
static const char VDashSemicolonEntityName[] = "VDash;";
static const char VbarSemicolonEntityName[] = "Vbar;";
static const char VcySemicolonEntityName[] = "Vcy;";
static const char VdashSemicolonEntityName[] = "Vdash;";
static const char VdashlSemicolonEntityName[] = "Vdashl;";
static const char VeeSemicolonEntityName[] = "Vee;";
static const char VerbarSemicolonEntityName[] = "Verbar;";
static const char VertSemicolonEntityName[] = "Vert;";
static const char VerticalBarSemicolonEntityName[] = "VerticalBar;";
static const char VerticalLineSemicolonEntityName[] = "VerticalLine;";
static const char VerticalSeparatorSemicolonEntityName[] = "VerticalSeparator;";
static const char VerticalTildeSemicolonEntityName[] = "VerticalTilde;";
static const char VeryThinSpaceSemicolonEntityName[] = "VeryThinSpace;";
static const char VfrSemicolonEntityName[] = "Vfr;";
static const char VopfSemicolonEntityName[] = "Vopf;";
static const char VscrSemicolonEntityName[] = "Vscr;";
static const char VvdashSemicolonEntityName[] = "Vvdash;";
static const char WcircSemicolonEntityName[] = "Wcirc;";
static const char WedgeSemicolonEntityName[] = "Wedge;";
static const char WfrSemicolonEntityName[] = "Wfr;";
static const char WopfSemicolonEntityName[] = "Wopf;";
static const char WscrSemicolonEntityName[] = "Wscr;";
static const char XfrSemicolonEntityName[] = "Xfr;";
static const char XiSemicolonEntityName[] = "Xi;";
static const char XopfSemicolonEntityName[] = "Xopf;";
static const char XscrSemicolonEntityName[] = "Xscr;";
static const char YAcySemicolonEntityName[] = "YAcy;";
static const char YIcySemicolonEntityName[] = "YIcy;";
static const char YUcySemicolonEntityName[] = "YUcy;";
static const char YacuteEntityName[] = "Yacute";
static const char YacuteSemicolonEntityName[] = "Yacute;";
static const char YcircSemicolonEntityName[] = "Ycirc;";
static const char YcySemicolonEntityName[] = "Ycy;";
static const char YfrSemicolonEntityName[] = "Yfr;";
static const char YopfSemicolonEntityName[] = "Yopf;";
static const char YscrSemicolonEntityName[] = "Yscr;";
static const char YumlSemicolonEntityName[] = "Yuml;";
static const char ZHcySemicolonEntityName[] = "ZHcy;";
static const char ZacuteSemicolonEntityName[] = "Zacute;";
static const char ZcaronSemicolonEntityName[] = "Zcaron;";
static const char ZcySemicolonEntityName[] = "Zcy;";
static const char ZdotSemicolonEntityName[] = "Zdot;";
static const char ZeroWidthSpaceSemicolonEntityName[] = "ZeroWidthSpace;";
static const char ZetaSemicolonEntityName[] = "Zeta;";
static const char ZfrSemicolonEntityName[] = "Zfr;";
static const char ZopfSemicolonEntityName[] = "Zopf;";
static const char ZscrSemicolonEntityName[] = "Zscr;";
static const char aacuteEntityName[] = "aacute";
static const char aacuteSemicolonEntityName[] = "aacute;";
static const char abreveSemicolonEntityName[] = "abreve;";
static const char acSemicolonEntityName[] = "ac;";
static const char acESemicolonEntityName[] = "acE;";
static const char acdSemicolonEntityName[] = "acd;";
static const char acircEntityName[] = "acirc";
static const char acircSemicolonEntityName[] = "acirc;";
static const char acuteEntityName[] = "acute";
static const char acuteSemicolonEntityName[] = "acute;";
static const char acySemicolonEntityName[] = "acy;";
static const char aeligEntityName[] = "aelig";
static const char aeligSemicolonEntityName[] = "aelig;";
static const char afSemicolonEntityName[] = "af;";
static const char afrSemicolonEntityName[] = "afr;";
static const char agraveEntityName[] = "agrave";
static const char agraveSemicolonEntityName[] = "agrave;";
static const char alefsymSemicolonEntityName[] = "alefsym;";
static const char alephSemicolonEntityName[] = "aleph;";
static const char alphaSemicolonEntityName[] = "alpha;";
static const char amacrSemicolonEntityName[] = "amacr;";
static const char amalgSemicolonEntityName[] = "amalg;";
static const char ampEntityName[] = "amp";
static const char ampSemicolonEntityName[] = "amp;";
static const char andSemicolonEntityName[] = "and;";
static const char andandSemicolonEntityName[] = "andand;";
static const char anddSemicolonEntityName[] = "andd;";
static const char andslopeSemicolonEntityName[] = "andslope;";
static const char andvSemicolonEntityName[] = "andv;";
static const char angSemicolonEntityName[] = "ang;";
static const char angeSemicolonEntityName[] = "ange;";
static const char angleSemicolonEntityName[] = "angle;";
static const char angmsdSemicolonEntityName[] = "angmsd;";
static const char angmsdaaSemicolonEntityName[] = "angmsdaa;";
static const char angmsdabSemicolonEntityName[] = "angmsdab;";
static const char angmsdacSemicolonEntityName[] = "angmsdac;";
static const char angmsdadSemicolonEntityName[] = "angmsdad;";
static const char angmsdaeSemicolonEntityName[] = "angmsdae;";
static const char angmsdafSemicolonEntityName[] = "angmsdaf;";
static const char angmsdagSemicolonEntityName[] = "angmsdag;";
static const char angmsdahSemicolonEntityName[] = "angmsdah;";
static const char angrtSemicolonEntityName[] = "angrt;";
static const char angrtvbSemicolonEntityName[] = "angrtvb;";
static const char angrtvbdSemicolonEntityName[] = "angrtvbd;";
static const char angsphSemicolonEntityName[] = "angsph;";
static const char angstSemicolonEntityName[] = "angst;";
static const char angzarrSemicolonEntityName[] = "angzarr;";
static const char aogonSemicolonEntityName[] = "aogon;";
static const char aopfSemicolonEntityName[] = "aopf;";
static const char apSemicolonEntityName[] = "ap;";
static const char apESemicolonEntityName[] = "apE;";
static const char apacirSemicolonEntityName[] = "apacir;";
static const char apeSemicolonEntityName[] = "ape;";
static const char apidSemicolonEntityName[] = "apid;";
static const char aposSemicolonEntityName[] = "apos;";
static const char approxSemicolonEntityName[] = "approx;";
static const char approxeqSemicolonEntityName[] = "approxeq;";
static const char aringEntityName[] = "aring";
static const char aringSemicolonEntityName[] = "aring;";
static const char ascrSemicolonEntityName[] = "ascr;";
static const char astSemicolonEntityName[] = "ast;";
static const char asympSemicolonEntityName[] = "asymp;";
static const char asympeqSemicolonEntityName[] = "asympeq;";
static const char atildeEntityName[] = "atilde";
static const char atildeSemicolonEntityName[] = "atilde;";
static const char aumlEntityName[] = "auml";
static const char aumlSemicolonEntityName[] = "auml;";
static const char awconintSemicolonEntityName[] = "awconint;";
static const char awintSemicolonEntityName[] = "awint;";
static const char bNotSemicolonEntityName[] = "bNot;";
static const char backcongSemicolonEntityName[] = "backcong;";
static const char backepsilonSemicolonEntityName[] = "backepsilon;";
static const char backprimeSemicolonEntityName[] = "backprime;";
static const char backsimSemicolonEntityName[] = "backsim;";
static const char backsimeqSemicolonEntityName[] = "backsimeq;";
static const char barveeSemicolonEntityName[] = "barvee;";
static const char barwedSemicolonEntityName[] = "barwed;";
static const char barwedgeSemicolonEntityName[] = "barwedge;";
static const char bbrkSemicolonEntityName[] = "bbrk;";
static const char bbrktbrkSemicolonEntityName[] = "bbrktbrk;";
static const char bcongSemicolonEntityName[] = "bcong;";
static const char bcySemicolonEntityName[] = "bcy;";
static const char bdquoSemicolonEntityName[] = "bdquo;";
static const char becausSemicolonEntityName[] = "becaus;";
static const char becauseSemicolonEntityName[] = "because;";
static const char bemptyvSemicolonEntityName[] = "bemptyv;";
static const char bepsiSemicolonEntityName[] = "bepsi;";
static const char bernouSemicolonEntityName[] = "bernou;";
static const char betaSemicolonEntityName[] = "beta;";
static const char bethSemicolonEntityName[] = "beth;";
static const char betweenSemicolonEntityName[] = "between;";
static const char bfrSemicolonEntityName[] = "bfr;";
static const char bigcapSemicolonEntityName[] = "bigcap;";
static const char bigcircSemicolonEntityName[] = "bigcirc;";
static const char bigcupSemicolonEntityName[] = "bigcup;";
static const char bigodotSemicolonEntityName[] = "bigodot;";
static const char bigoplusSemicolonEntityName[] = "bigoplus;";
static const char bigotimesSemicolonEntityName[] = "bigotimes;";
static const char bigsqcupSemicolonEntityName[] = "bigsqcup;";
static const char bigstarSemicolonEntityName[] = "bigstar;";
static const char bigtriangledownSemicolonEntityName[] = "bigtriangledown;";
static const char bigtriangleupSemicolonEntityName[] = "bigtriangleup;";
static const char biguplusSemicolonEntityName[] = "biguplus;";
static const char bigveeSemicolonEntityName[] = "bigvee;";
static const char bigwedgeSemicolonEntityName[] = "bigwedge;";
static const char bkarowSemicolonEntityName[] = "bkarow;";
static const char blacklozengeSemicolonEntityName[] = "blacklozenge;";
static const char blacksquareSemicolonEntityName[] = "blacksquare;";
static const char blacktriangleSemicolonEntityName[] = "blacktriangle;";
static const char blacktriangledownSemicolonEntityName[] = "blacktriangledown;";
static const char blacktriangleleftSemicolonEntityName[] = "blacktriangleleft;";
static const char blacktrianglerightSemicolonEntityName[] = "blacktriangleright;";
static const char blankSemicolonEntityName[] = "blank;";
static const char blk12SemicolonEntityName[] = "blk12;";
static const char blk14SemicolonEntityName[] = "blk14;";
static const char blk34SemicolonEntityName[] = "blk34;";
static const char blockSemicolonEntityName[] = "block;";
static const char bneSemicolonEntityName[] = "bne;";
static const char bnequivSemicolonEntityName[] = "bnequiv;";
static const char bnotSemicolonEntityName[] = "bnot;";
static const char bopfSemicolonEntityName[] = "bopf;";
static const char botSemicolonEntityName[] = "bot;";
static const char bottomSemicolonEntityName[] = "bottom;";
static const char bowtieSemicolonEntityName[] = "bowtie;";
static const char boxDLSemicolonEntityName[] = "boxDL;";
static const char boxDRSemicolonEntityName[] = "boxDR;";
static const char boxDlSemicolonEntityName[] = "boxDl;";
static const char boxDrSemicolonEntityName[] = "boxDr;";
static const char boxHSemicolonEntityName[] = "boxH;";
static const char boxHDSemicolonEntityName[] = "boxHD;";
static const char boxHUSemicolonEntityName[] = "boxHU;";
static const char boxHdSemicolonEntityName[] = "boxHd;";
static const char boxHuSemicolonEntityName[] = "boxHu;";
static const char boxULSemicolonEntityName[] = "boxUL;";
static const char boxURSemicolonEntityName[] = "boxUR;";
static const char boxUlSemicolonEntityName[] = "boxUl;";
static const char boxUrSemicolonEntityName[] = "boxUr;";
static const char boxVSemicolonEntityName[] = "boxV;";
static const char boxVHSemicolonEntityName[] = "boxVH;";
static const char boxVLSemicolonEntityName[] = "boxVL;";
static const char boxVRSemicolonEntityName[] = "boxVR;";
static const char boxVhSemicolonEntityName[] = "boxVh;";
static const char boxVlSemicolonEntityName[] = "boxVl;";
static const char boxVrSemicolonEntityName[] = "boxVr;";
static const char boxboxSemicolonEntityName[] = "boxbox;";
static const char boxdLSemicolonEntityName[] = "boxdL;";
static const char boxdRSemicolonEntityName[] = "boxdR;";
static const char boxdlSemicolonEntityName[] = "boxdl;";
static const char boxdrSemicolonEntityName[] = "boxdr;";
static const char boxhSemicolonEntityName[] = "boxh;";
static const char boxhDSemicolonEntityName[] = "boxhD;";
static const char boxhUSemicolonEntityName[] = "boxhU;";
static const char boxhdSemicolonEntityName[] = "boxhd;";
static const char boxhuSemicolonEntityName[] = "boxhu;";
static const char boxminusSemicolonEntityName[] = "boxminus;";
static const char boxplusSemicolonEntityName[] = "boxplus;";
static const char boxtimesSemicolonEntityName[] = "boxtimes;";
static const char boxuLSemicolonEntityName[] = "boxuL;";
static const char boxuRSemicolonEntityName[] = "boxuR;";
static const char boxulSemicolonEntityName[] = "boxul;";
static const char boxurSemicolonEntityName[] = "boxur;";
static const char boxvSemicolonEntityName[] = "boxv;";
static const char boxvHSemicolonEntityName[] = "boxvH;";
static const char boxvLSemicolonEntityName[] = "boxvL;";
static const char boxvRSemicolonEntityName[] = "boxvR;";
static const char boxvhSemicolonEntityName[] = "boxvh;";
static const char boxvlSemicolonEntityName[] = "boxvl;";
static const char boxvrSemicolonEntityName[] = "boxvr;";
static const char bprimeSemicolonEntityName[] = "bprime;";
static const char breveSemicolonEntityName[] = "breve;";
static const char brvbarEntityName[] = "brvbar";
static const char brvbarSemicolonEntityName[] = "brvbar;";
static const char bscrSemicolonEntityName[] = "bscr;";
static const char bsemiSemicolonEntityName[] = "bsemi;";
static const char bsimSemicolonEntityName[] = "bsim;";
static const char bsimeSemicolonEntityName[] = "bsime;";
static const char bsolSemicolonEntityName[] = "bsol;";
static const char bsolbSemicolonEntityName[] = "bsolb;";
static const char bsolhsubSemicolonEntityName[] = "bsolhsub;";
static const char bullSemicolonEntityName[] = "bull;";
static const char bulletSemicolonEntityName[] = "bullet;";
static const char bumpSemicolonEntityName[] = "bump;";
static const char bumpESemicolonEntityName[] = "bumpE;";
static const char bumpeSemicolonEntityName[] = "bumpe;";
static const char bumpeqSemicolonEntityName[] = "bumpeq;";
static const char cacuteSemicolonEntityName[] = "cacute;";
static const char capSemicolonEntityName[] = "cap;";
static const char capandSemicolonEntityName[] = "capand;";
static const char capbrcupSemicolonEntityName[] = "capbrcup;";
static const char capcapSemicolonEntityName[] = "capcap;";
static const char capcupSemicolonEntityName[] = "capcup;";
static const char capdotSemicolonEntityName[] = "capdot;";
static const char capsSemicolonEntityName[] = "caps;";
static const char caretSemicolonEntityName[] = "caret;";
static const char caronSemicolonEntityName[] = "caron;";
static const char ccapsSemicolonEntityName[] = "ccaps;";
static const char ccaronSemicolonEntityName[] = "ccaron;";
static const char ccedilEntityName[] = "ccedil";
static const char ccedilSemicolonEntityName[] = "ccedil;";
static const char ccircSemicolonEntityName[] = "ccirc;";
static const char ccupsSemicolonEntityName[] = "ccups;";
static const char ccupssmSemicolonEntityName[] = "ccupssm;";
static const char cdotSemicolonEntityName[] = "cdot;";
static const char cedilEntityName[] = "cedil";
static const char cedilSemicolonEntityName[] = "cedil;";
static const char cemptyvSemicolonEntityName[] = "cemptyv;";
static const char centEntityName[] = "cent";
static const char centSemicolonEntityName[] = "cent;";
static const char centerdotSemicolonEntityName[] = "centerdot;";
static const char cfrSemicolonEntityName[] = "cfr;";
static const char chcySemicolonEntityName[] = "chcy;";
static const char checkSemicolonEntityName[] = "check;";
static const char checkmarkSemicolonEntityName[] = "checkmark;";
static const char chiSemicolonEntityName[] = "chi;";
static const char cirSemicolonEntityName[] = "cir;";
static const char cirESemicolonEntityName[] = "cirE;";
static const char circSemicolonEntityName[] = "circ;";
static const char circeqSemicolonEntityName[] = "circeq;";
static const char circlearrowleftSemicolonEntityName[] = "circlearrowleft;";
static const char circlearrowrightSemicolonEntityName[] = "circlearrowright;";
static const char circledRSemicolonEntityName[] = "circledR;";
static const char circledSSemicolonEntityName[] = "circledS;";
static const char circledastSemicolonEntityName[] = "circledast;";
static const char circledcircSemicolonEntityName[] = "circledcirc;";
static const char circleddashSemicolonEntityName[] = "circleddash;";
static const char cireSemicolonEntityName[] = "cire;";
static const char cirfnintSemicolonEntityName[] = "cirfnint;";
static const char cirmidSemicolonEntityName[] = "cirmid;";
static const char cirscirSemicolonEntityName[] = "cirscir;";
static const char clubsSemicolonEntityName[] = "clubs;";
static const char clubsuitSemicolonEntityName[] = "clubsuit;";
static const char colonSemicolonEntityName[] = "colon;";
static const char coloneSemicolonEntityName[] = "colone;";
static const char coloneqSemicolonEntityName[] = "coloneq;";
static const char commaSemicolonEntityName[] = "comma;";
static const char commatSemicolonEntityName[] = "commat;";
static const char compSemicolonEntityName[] = "comp;";
static const char compfnSemicolonEntityName[] = "compfn;";
static const char complementSemicolonEntityName[] = "complement;";
static const char complexesSemicolonEntityName[] = "complexes;";
static const char congSemicolonEntityName[] = "cong;";
static const char congdotSemicolonEntityName[] = "congdot;";
static const char conintSemicolonEntityName[] = "conint;";
static const char copfSemicolonEntityName[] = "copf;";
static const char coprodSemicolonEntityName[] = "coprod;";
static const char copyEntityName[] = "copy";
static const char copySemicolonEntityName[] = "copy;";
static const char copysrSemicolonEntityName[] = "copysr;";
static const char crarrSemicolonEntityName[] = "crarr;";
static const char crossSemicolonEntityName[] = "cross;";
static const char cscrSemicolonEntityName[] = "cscr;";
static const char csubSemicolonEntityName[] = "csub;";
static const char csubeSemicolonEntityName[] = "csube;";
static const char csupSemicolonEntityName[] = "csup;";
static const char csupeSemicolonEntityName[] = "csupe;";
static const char ctdotSemicolonEntityName[] = "ctdot;";
static const char cudarrlSemicolonEntityName[] = "cudarrl;";
static const char cudarrrSemicolonEntityName[] = "cudarrr;";
static const char cueprSemicolonEntityName[] = "cuepr;";
static const char cuescSemicolonEntityName[] = "cuesc;";
static const char cularrSemicolonEntityName[] = "cularr;";
static const char cularrpSemicolonEntityName[] = "cularrp;";
static const char cupSemicolonEntityName[] = "cup;";
static const char cupbrcapSemicolonEntityName[] = "cupbrcap;";
static const char cupcapSemicolonEntityName[] = "cupcap;";
static const char cupcupSemicolonEntityName[] = "cupcup;";
static const char cupdotSemicolonEntityName[] = "cupdot;";
static const char cuporSemicolonEntityName[] = "cupor;";
static const char cupsSemicolonEntityName[] = "cups;";
static const char curarrSemicolonEntityName[] = "curarr;";
static const char curarrmSemicolonEntityName[] = "curarrm;";
static const char curlyeqprecSemicolonEntityName[] = "curlyeqprec;";
static const char curlyeqsuccSemicolonEntityName[] = "curlyeqsucc;";
static const char curlyveeSemicolonEntityName[] = "curlyvee;";
static const char curlywedgeSemicolonEntityName[] = "curlywedge;";
static const char currenEntityName[] = "curren";
static const char currenSemicolonEntityName[] = "curren;";
static const char curvearrowleftSemicolonEntityName[] = "curvearrowleft;";
static const char curvearrowrightSemicolonEntityName[] = "curvearrowright;";
static const char cuveeSemicolonEntityName[] = "cuvee;";
static const char cuwedSemicolonEntityName[] = "cuwed;";
static const char cwconintSemicolonEntityName[] = "cwconint;";
static const char cwintSemicolonEntityName[] = "cwint;";
static const char cylctySemicolonEntityName[] = "cylcty;";
static const char dArrSemicolonEntityName[] = "dArr;";
static const char dHarSemicolonEntityName[] = "dHar;";
static const char daggerSemicolonEntityName[] = "dagger;";
static const char dalethSemicolonEntityName[] = "daleth;";
static const char darrSemicolonEntityName[] = "darr;";
static const char dashSemicolonEntityName[] = "dash;";
static const char dashvSemicolonEntityName[] = "dashv;";
static const char dbkarowSemicolonEntityName[] = "dbkarow;";
static const char dblacSemicolonEntityName[] = "dblac;";
static const char dcaronSemicolonEntityName[] = "dcaron;";
static const char dcySemicolonEntityName[] = "dcy;";
static const char ddSemicolonEntityName[] = "dd;";
static const char ddaggerSemicolonEntityName[] = "ddagger;";
static const char ddarrSemicolonEntityName[] = "ddarr;";
static const char ddotseqSemicolonEntityName[] = "ddotseq;";
static const char degEntityName[] = "deg";
static const char degSemicolonEntityName[] = "deg;";
static const char deltaSemicolonEntityName[] = "delta;";
static const char demptyvSemicolonEntityName[] = "demptyv;";
static const char dfishtSemicolonEntityName[] = "dfisht;";
static const char dfrSemicolonEntityName[] = "dfr;";
static const char dharlSemicolonEntityName[] = "dharl;";
static const char dharrSemicolonEntityName[] = "dharr;";
static const char diamSemicolonEntityName[] = "diam;";
static const char diamondSemicolonEntityName[] = "diamond;";
static const char diamondsuitSemicolonEntityName[] = "diamondsuit;";
static const char diamsSemicolonEntityName[] = "diams;";
static const char dieSemicolonEntityName[] = "die;";
static const char digammaSemicolonEntityName[] = "digamma;";
static const char disinSemicolonEntityName[] = "disin;";
static const char divSemicolonEntityName[] = "div;";
static const char divideEntityName[] = "divide";
static const char divideSemicolonEntityName[] = "divide;";
static const char divideontimesSemicolonEntityName[] = "divideontimes;";
static const char divonxSemicolonEntityName[] = "divonx;";
static const char djcySemicolonEntityName[] = "djcy;";
static const char dlcornSemicolonEntityName[] = "dlcorn;";
static const char dlcropSemicolonEntityName[] = "dlcrop;";
static const char dollarSemicolonEntityName[] = "dollar;";
static const char dopfSemicolonEntityName[] = "dopf;";
static const char dotSemicolonEntityName[] = "dot;";
static const char doteqSemicolonEntityName[] = "doteq;";
static const char doteqdotSemicolonEntityName[] = "doteqdot;";
static const char dotminusSemicolonEntityName[] = "dotminus;";
static const char dotplusSemicolonEntityName[] = "dotplus;";
static const char dotsquareSemicolonEntityName[] = "dotsquare;";
static const char doublebarwedgeSemicolonEntityName[] = "doublebarwedge;";
static const char downarrowSemicolonEntityName[] = "downarrow;";
static const char downdownarrowsSemicolonEntityName[] = "downdownarrows;";
static const char downharpoonleftSemicolonEntityName[] = "downharpoonleft;";
static const char downharpoonrightSemicolonEntityName[] = "downharpoonright;";
static const char drbkarowSemicolonEntityName[] = "drbkarow;";
static const char drcornSemicolonEntityName[] = "drcorn;";
static const char drcropSemicolonEntityName[] = "drcrop;";
static const char dscrSemicolonEntityName[] = "dscr;";
static const char dscySemicolonEntityName[] = "dscy;";
static const char dsolSemicolonEntityName[] = "dsol;";
static const char dstrokSemicolonEntityName[] = "dstrok;";
static const char dtdotSemicolonEntityName[] = "dtdot;";
static const char dtriSemicolonEntityName[] = "dtri;";
static const char dtrifSemicolonEntityName[] = "dtrif;";
static const char duarrSemicolonEntityName[] = "duarr;";
static const char duharSemicolonEntityName[] = "duhar;";
static const char dwangleSemicolonEntityName[] = "dwangle;";
static const char dzcySemicolonEntityName[] = "dzcy;";
static const char dzigrarrSemicolonEntityName[] = "dzigrarr;";
static const char eDDotSemicolonEntityName[] = "eDDot;";
static const char eDotSemicolonEntityName[] = "eDot;";
static const char eacuteEntityName[] = "eacute";
static const char eacuteSemicolonEntityName[] = "eacute;";
static const char easterSemicolonEntityName[] = "easter;";
static const char ecaronSemicolonEntityName[] = "ecaron;";
static const char ecirSemicolonEntityName[] = "ecir;";
static const char ecircEntityName[] = "ecirc";
static const char ecircSemicolonEntityName[] = "ecirc;";
static const char ecolonSemicolonEntityName[] = "ecolon;";
static const char ecySemicolonEntityName[] = "ecy;";
static const char edotSemicolonEntityName[] = "edot;";
static const char eeSemicolonEntityName[] = "ee;";
static const char efDotSemicolonEntityName[] = "efDot;";
static const char efrSemicolonEntityName[] = "efr;";
static const char egSemicolonEntityName[] = "eg;";
static const char egraveEntityName[] = "egrave";
static const char egraveSemicolonEntityName[] = "egrave;";
static const char egsSemicolonEntityName[] = "egs;";
static const char egsdotSemicolonEntityName[] = "egsdot;";
static const char elSemicolonEntityName[] = "el;";
static const char elintersSemicolonEntityName[] = "elinters;";
static const char ellSemicolonEntityName[] = "ell;";
static const char elsSemicolonEntityName[] = "els;";
static const char elsdotSemicolonEntityName[] = "elsdot;";
static const char emacrSemicolonEntityName[] = "emacr;";
static const char emptySemicolonEntityName[] = "empty;";
static const char emptysetSemicolonEntityName[] = "emptyset;";
static const char emptyvSemicolonEntityName[] = "emptyv;";
static const char emsp13SemicolonEntityName[] = "emsp13;";
static const char emsp14SemicolonEntityName[] = "emsp14;";
static const char emspSemicolonEntityName[] = "emsp;";
static const char engSemicolonEntityName[] = "eng;";
static const char enspSemicolonEntityName[] = "ensp;";
static const char eogonSemicolonEntityName[] = "eogon;";
static const char eopfSemicolonEntityName[] = "eopf;";
static const char eparSemicolonEntityName[] = "epar;";
static const char eparslSemicolonEntityName[] = "eparsl;";
static const char eplusSemicolonEntityName[] = "eplus;";
static const char epsiSemicolonEntityName[] = "epsi;";
static const char epsilonSemicolonEntityName[] = "epsilon;";
static const char epsivSemicolonEntityName[] = "epsiv;";
static const char eqcircSemicolonEntityName[] = "eqcirc;";
static const char eqcolonSemicolonEntityName[] = "eqcolon;";
static const char eqsimSemicolonEntityName[] = "eqsim;";
static const char eqslantgtrSemicolonEntityName[] = "eqslantgtr;";
static const char eqslantlessSemicolonEntityName[] = "eqslantless;";
static const char equalsSemicolonEntityName[] = "equals;";
static const char equestSemicolonEntityName[] = "equest;";
static const char equivSemicolonEntityName[] = "equiv;";
static const char equivDDSemicolonEntityName[] = "equivDD;";
static const char eqvparslSemicolonEntityName[] = "eqvparsl;";
static const char erDotSemicolonEntityName[] = "erDot;";
static const char erarrSemicolonEntityName[] = "erarr;";
static const char escrSemicolonEntityName[] = "escr;";
static const char esdotSemicolonEntityName[] = "esdot;";
static const char esimSemicolonEntityName[] = "esim;";
static const char etaSemicolonEntityName[] = "eta;";
static const char ethEntityName[] = "eth";
static const char ethSemicolonEntityName[] = "eth;";
static const char eumlEntityName[] = "euml";
static const char eumlSemicolonEntityName[] = "euml;";
static const char euroSemicolonEntityName[] = "euro;";
static const char exclSemicolonEntityName[] = "excl;";
static const char existSemicolonEntityName[] = "exist;";
static const char expectationSemicolonEntityName[] = "expectation;";
static const char exponentialeSemicolonEntityName[] = "exponentiale;";
static const char fallingdotseqSemicolonEntityName[] = "fallingdotseq;";
static const char fcySemicolonEntityName[] = "fcy;";
static const char femaleSemicolonEntityName[] = "female;";
static const char ffiligSemicolonEntityName[] = "ffilig;";
static const char ffligSemicolonEntityName[] = "fflig;";
static const char fflligSemicolonEntityName[] = "ffllig;";
static const char ffrSemicolonEntityName[] = "ffr;";
static const char filigSemicolonEntityName[] = "filig;";
static const char fjligSemicolonEntityName[] = "fjlig;";
static const char flatSemicolonEntityName[] = "flat;";
static const char flligSemicolonEntityName[] = "fllig;";
static const char fltnsSemicolonEntityName[] = "fltns;";
static const char fnofSemicolonEntityName[] = "fnof;";
static const char fopfSemicolonEntityName[] = "fopf;";
static const char forallSemicolonEntityName[] = "forall;";
static const char forkSemicolonEntityName[] = "fork;";
static const char forkvSemicolonEntityName[] = "forkv;";
static const char fpartintSemicolonEntityName[] = "fpartint;";
static const char frac12EntityName[] = "frac12";
static const char frac12SemicolonEntityName[] = "frac12;";
static const char frac13SemicolonEntityName[] = "frac13;";
static const char frac14EntityName[] = "frac14";
static const char frac14SemicolonEntityName[] = "frac14;";
static const char frac15SemicolonEntityName[] = "frac15;";
static const char frac16SemicolonEntityName[] = "frac16;";
static const char frac18SemicolonEntityName[] = "frac18;";
static const char frac23SemicolonEntityName[] = "frac23;";
static const char frac25SemicolonEntityName[] = "frac25;";
static const char frac34EntityName[] = "frac34";
static const char frac34SemicolonEntityName[] = "frac34;";
static const char frac35SemicolonEntityName[] = "frac35;";
static const char frac38SemicolonEntityName[] = "frac38;";
static const char frac45SemicolonEntityName[] = "frac45;";
static const char frac56SemicolonEntityName[] = "frac56;";
static const char frac58SemicolonEntityName[] = "frac58;";
static const char frac78SemicolonEntityName[] = "frac78;";
static const char fraslSemicolonEntityName[] = "frasl;";
static const char frownSemicolonEntityName[] = "frown;";
static const char fscrSemicolonEntityName[] = "fscr;";
static const char gESemicolonEntityName[] = "gE;";
static const char gElSemicolonEntityName[] = "gEl;";
static const char gacuteSemicolonEntityName[] = "gacute;";
static const char gammaSemicolonEntityName[] = "gamma;";
static const char gammadSemicolonEntityName[] = "gammad;";
static const char gapSemicolonEntityName[] = "gap;";
static const char gbreveSemicolonEntityName[] = "gbreve;";
static const char gcircSemicolonEntityName[] = "gcirc;";
static const char gcySemicolonEntityName[] = "gcy;";
static const char gdotSemicolonEntityName[] = "gdot;";
static const char geSemicolonEntityName[] = "ge;";
static const char gelSemicolonEntityName[] = "gel;";
static const char geqSemicolonEntityName[] = "geq;";
static const char geqqSemicolonEntityName[] = "geqq;";
static const char geqslantSemicolonEntityName[] = "geqslant;";
static const char gesSemicolonEntityName[] = "ges;";
static const char gesccSemicolonEntityName[] = "gescc;";
static const char gesdotSemicolonEntityName[] = "gesdot;";
static const char gesdotoSemicolonEntityName[] = "gesdoto;";
static const char gesdotolSemicolonEntityName[] = "gesdotol;";
static const char geslSemicolonEntityName[] = "gesl;";
static const char geslesSemicolonEntityName[] = "gesles;";
static const char gfrSemicolonEntityName[] = "gfr;";
static const char ggSemicolonEntityName[] = "gg;";
static const char gggSemicolonEntityName[] = "ggg;";
static const char gimelSemicolonEntityName[] = "gimel;";
static const char gjcySemicolonEntityName[] = "gjcy;";
static const char glSemicolonEntityName[] = "gl;";
static const char glESemicolonEntityName[] = "glE;";
static const char glaSemicolonEntityName[] = "gla;";
static const char gljSemicolonEntityName[] = "glj;";
static const char gnESemicolonEntityName[] = "gnE;";
static const char gnapSemicolonEntityName[] = "gnap;";
static const char gnapproxSemicolonEntityName[] = "gnapprox;";
static const char gneSemicolonEntityName[] = "gne;";
static const char gneqSemicolonEntityName[] = "gneq;";
static const char gneqqSemicolonEntityName[] = "gneqq;";
static const char gnsimSemicolonEntityName[] = "gnsim;";
static const char gopfSemicolonEntityName[] = "gopf;";
static const char graveSemicolonEntityName[] = "grave;";
static const char gscrSemicolonEntityName[] = "gscr;";
static const char gsimSemicolonEntityName[] = "gsim;";
static const char gsimeSemicolonEntityName[] = "gsime;";
static const char gsimlSemicolonEntityName[] = "gsiml;";
static const char gtEntityName[] = "gt";
static const char gtSemicolonEntityName[] = "gt;";
static const char gtccSemicolonEntityName[] = "gtcc;";
static const char gtcirSemicolonEntityName[] = "gtcir;";
static const char gtdotSemicolonEntityName[] = "gtdot;";
static const char gtlParSemicolonEntityName[] = "gtlPar;";
static const char gtquestSemicolonEntityName[] = "gtquest;";
static const char gtrapproxSemicolonEntityName[] = "gtrapprox;";
static const char gtrarrSemicolonEntityName[] = "gtrarr;";
static const char gtrdotSemicolonEntityName[] = "gtrdot;";
static const char gtreqlessSemicolonEntityName[] = "gtreqless;";
static const char gtreqqlessSemicolonEntityName[] = "gtreqqless;";
static const char gtrlessSemicolonEntityName[] = "gtrless;";
static const char gtrsimSemicolonEntityName[] = "gtrsim;";
static const char gvertneqqSemicolonEntityName[] = "gvertneqq;";
static const char gvnESemicolonEntityName[] = "gvnE;";
static const char hArrSemicolonEntityName[] = "hArr;";
static const char hairspSemicolonEntityName[] = "hairsp;";
static const char halfSemicolonEntityName[] = "half;";
static const char hamiltSemicolonEntityName[] = "hamilt;";
static const char hardcySemicolonEntityName[] = "hardcy;";
static const char harrSemicolonEntityName[] = "harr;";
static const char harrcirSemicolonEntityName[] = "harrcir;";
static const char harrwSemicolonEntityName[] = "harrw;";
static const char hbarSemicolonEntityName[] = "hbar;";
static const char hcircSemicolonEntityName[] = "hcirc;";
static const char heartsSemicolonEntityName[] = "hearts;";
static const char heartsuitSemicolonEntityName[] = "heartsuit;";
static const char hellipSemicolonEntityName[] = "hellip;";
static const char herconSemicolonEntityName[] = "hercon;";
static const char hfrSemicolonEntityName[] = "hfr;";
static const char hksearowSemicolonEntityName[] = "hksearow;";
static const char hkswarowSemicolonEntityName[] = "hkswarow;";
static const char hoarrSemicolonEntityName[] = "hoarr;";
static const char homthtSemicolonEntityName[] = "homtht;";
static const char hookleftarrowSemicolonEntityName[] = "hookleftarrow;";
static const char hookrightarrowSemicolonEntityName[] = "hookrightarrow;";
static const char hopfSemicolonEntityName[] = "hopf;";
static const char horbarSemicolonEntityName[] = "horbar;";
static const char hscrSemicolonEntityName[] = "hscr;";
static const char hslashSemicolonEntityName[] = "hslash;";
static const char hstrokSemicolonEntityName[] = "hstrok;";
static const char hybullSemicolonEntityName[] = "hybull;";
static const char hyphenSemicolonEntityName[] = "hyphen;";
static const char iacuteEntityName[] = "iacute";
static const char iacuteSemicolonEntityName[] = "iacute;";
static const char icSemicolonEntityName[] = "ic;";
static const char icircEntityName[] = "icirc";
static const char icircSemicolonEntityName[] = "icirc;";
static const char icySemicolonEntityName[] = "icy;";
static const char iecySemicolonEntityName[] = "iecy;";
static const char iexclEntityName[] = "iexcl";
static const char iexclSemicolonEntityName[] = "iexcl;";
static const char iffSemicolonEntityName[] = "iff;";
static const char ifrSemicolonEntityName[] = "ifr;";
static const char igraveEntityName[] = "igrave";
static const char igraveSemicolonEntityName[] = "igrave;";
static const char iiSemicolonEntityName[] = "ii;";
static const char iiiintSemicolonEntityName[] = "iiiint;";
static const char iiintSemicolonEntityName[] = "iiint;";
static const char iinfinSemicolonEntityName[] = "iinfin;";
static const char iiotaSemicolonEntityName[] = "iiota;";
static const char ijligSemicolonEntityName[] = "ijlig;";
static const char imacrSemicolonEntityName[] = "imacr;";
static const char imageSemicolonEntityName[] = "image;";
static const char imaglineSemicolonEntityName[] = "imagline;";
static const char imagpartSemicolonEntityName[] = "imagpart;";
static const char imathSemicolonEntityName[] = "imath;";
static const char imofSemicolonEntityName[] = "imof;";
static const char impedSemicolonEntityName[] = "imped;";
static const char inSemicolonEntityName[] = "in;";
static const char incareSemicolonEntityName[] = "incare;";
static const char infinSemicolonEntityName[] = "infin;";
static const char infintieSemicolonEntityName[] = "infintie;";
static const char inodotSemicolonEntityName[] = "inodot;";
static const char intSemicolonEntityName[] = "int;";
static const char intcalSemicolonEntityName[] = "intcal;";
static const char integersSemicolonEntityName[] = "integers;";
static const char intercalSemicolonEntityName[] = "intercal;";
static const char intlarhkSemicolonEntityName[] = "intlarhk;";
static const char intprodSemicolonEntityName[] = "intprod;";
static const char iocySemicolonEntityName[] = "iocy;";
static const char iogonSemicolonEntityName[] = "iogon;";
static const char iopfSemicolonEntityName[] = "iopf;";
static const char iotaSemicolonEntityName[] = "iota;";
static const char iprodSemicolonEntityName[] = "iprod;";
static const char iquestEntityName[] = "iquest";
static const char iquestSemicolonEntityName[] = "iquest;";
static const char iscrSemicolonEntityName[] = "iscr;";
static const char isinSemicolonEntityName[] = "isin;";
static const char isinESemicolonEntityName[] = "isinE;";
static const char isindotSemicolonEntityName[] = "isindot;";
static const char isinsSemicolonEntityName[] = "isins;";
static const char isinsvSemicolonEntityName[] = "isinsv;";
static const char isinvSemicolonEntityName[] = "isinv;";
static const char itSemicolonEntityName[] = "it;";
static const char itildeSemicolonEntityName[] = "itilde;";
static const char iukcySemicolonEntityName[] = "iukcy;";
static const char iumlEntityName[] = "iuml";
static const char iumlSemicolonEntityName[] = "iuml;";
static const char jcircSemicolonEntityName[] = "jcirc;";
static const char jcySemicolonEntityName[] = "jcy;";
static const char jfrSemicolonEntityName[] = "jfr;";
static const char jmathSemicolonEntityName[] = "jmath;";
static const char jopfSemicolonEntityName[] = "jopf;";
static const char jscrSemicolonEntityName[] = "jscr;";
static const char jsercySemicolonEntityName[] = "jsercy;";
static const char jukcySemicolonEntityName[] = "jukcy;";
static const char kappaSemicolonEntityName[] = "kappa;";
static const char kappavSemicolonEntityName[] = "kappav;";
static const char kcedilSemicolonEntityName[] = "kcedil;";
static const char kcySemicolonEntityName[] = "kcy;";
static const char kfrSemicolonEntityName[] = "kfr;";
static const char kgreenSemicolonEntityName[] = "kgreen;";
static const char khcySemicolonEntityName[] = "khcy;";
static const char kjcySemicolonEntityName[] = "kjcy;";
static const char kopfSemicolonEntityName[] = "kopf;";
static const char kscrSemicolonEntityName[] = "kscr;";
static const char lAarrSemicolonEntityName[] = "lAarr;";
static const char lArrSemicolonEntityName[] = "lArr;";
static const char lAtailSemicolonEntityName[] = "lAtail;";
static const char lBarrSemicolonEntityName[] = "lBarr;";
static const char lESemicolonEntityName[] = "lE;";
static const char lEgSemicolonEntityName[] = "lEg;";
static const char lHarSemicolonEntityName[] = "lHar;";
static const char lacuteSemicolonEntityName[] = "lacute;";
static const char laemptyvSemicolonEntityName[] = "laemptyv;";
static const char lagranSemicolonEntityName[] = "lagran;";
static const char lambdaSemicolonEntityName[] = "lambda;";
static const char langSemicolonEntityName[] = "lang;";
static const char langdSemicolonEntityName[] = "langd;";
static const char langleSemicolonEntityName[] = "langle;";
static const char lapSemicolonEntityName[] = "lap;";
static const char laquoEntityName[] = "laquo";
static const char laquoSemicolonEntityName[] = "laquo;";
static const char larrSemicolonEntityName[] = "larr;";
static const char larrbSemicolonEntityName[] = "larrb;";
static const char larrbfsSemicolonEntityName[] = "larrbfs;";
static const char larrfsSemicolonEntityName[] = "larrfs;";
static const char larrhkSemicolonEntityName[] = "larrhk;";
static const char larrlpSemicolonEntityName[] = "larrlp;";
static const char larrplSemicolonEntityName[] = "larrpl;";
static const char larrsimSemicolonEntityName[] = "larrsim;";
static const char larrtlSemicolonEntityName[] = "larrtl;";
static const char latSemicolonEntityName[] = "lat;";
static const char latailSemicolonEntityName[] = "latail;";
static const char lateSemicolonEntityName[] = "late;";
static const char latesSemicolonEntityName[] = "lates;";
static const char lbarrSemicolonEntityName[] = "lbarr;";
static const char lbbrkSemicolonEntityName[] = "lbbrk;";
static const char lbraceSemicolonEntityName[] = "lbrace;";
static const char lbrackSemicolonEntityName[] = "lbrack;";
static const char lbrkeSemicolonEntityName[] = "lbrke;";
static const char lbrksldSemicolonEntityName[] = "lbrksld;";
static const char lbrksluSemicolonEntityName[] = "lbrkslu;";
static const char lcaronSemicolonEntityName[] = "lcaron;";
static const char lcedilSemicolonEntityName[] = "lcedil;";
static const char lceilSemicolonEntityName[] = "lceil;";
static const char lcubSemicolonEntityName[] = "lcub;";
static const char lcySemicolonEntityName[] = "lcy;";
static const char ldcaSemicolonEntityName[] = "ldca;";
static const char ldquoSemicolonEntityName[] = "ldquo;";
static const char ldquorSemicolonEntityName[] = "ldquor;";
static const char ldrdharSemicolonEntityName[] = "ldrdhar;";
static const char ldrusharSemicolonEntityName[] = "ldrushar;";
static const char ldshSemicolonEntityName[] = "ldsh;";
static const char leSemicolonEntityName[] = "le;";
static const char leftarrowSemicolonEntityName[] = "leftarrow;";
static const char leftarrowtailSemicolonEntityName[] = "leftarrowtail;";
static const char leftharpoondownSemicolonEntityName[] = "leftharpoondown;";
static const char leftharpoonupSemicolonEntityName[] = "leftharpoonup;";
static const char leftleftarrowsSemicolonEntityName[] = "leftleftarrows;";
static const char leftrightarrowSemicolonEntityName[] = "leftrightarrow;";
static const char leftrightarrowsSemicolonEntityName[] = "leftrightarrows;";
static const char leftrightharpoonsSemicolonEntityName[] = "leftrightharpoons;";
static const char leftrightsquigarrowSemicolonEntityName[] = "leftrightsquigarrow;";
static const char leftthreetimesSemicolonEntityName[] = "leftthreetimes;";
static const char legSemicolonEntityName[] = "leg;";
static const char leqSemicolonEntityName[] = "leq;";
static const char leqqSemicolonEntityName[] = "leqq;";
static const char leqslantSemicolonEntityName[] = "leqslant;";
static const char lesSemicolonEntityName[] = "les;";
static const char lesccSemicolonEntityName[] = "lescc;";
static const char lesdotSemicolonEntityName[] = "lesdot;";
static const char lesdotoSemicolonEntityName[] = "lesdoto;";
static const char lesdotorSemicolonEntityName[] = "lesdotor;";
static const char lesgSemicolonEntityName[] = "lesg;";
static const char lesgesSemicolonEntityName[] = "lesges;";
static const char lessapproxSemicolonEntityName[] = "lessapprox;";
static const char lessdotSemicolonEntityName[] = "lessdot;";
static const char lesseqgtrSemicolonEntityName[] = "lesseqgtr;";
static const char lesseqqgtrSemicolonEntityName[] = "lesseqqgtr;";
static const char lessgtrSemicolonEntityName[] = "lessgtr;";
static const char lesssimSemicolonEntityName[] = "lesssim;";
static const char lfishtSemicolonEntityName[] = "lfisht;";
static const char lfloorSemicolonEntityName[] = "lfloor;";
static const char lfrSemicolonEntityName[] = "lfr;";
static const char lgSemicolonEntityName[] = "lg;";
static const char lgESemicolonEntityName[] = "lgE;";
static const char lhardSemicolonEntityName[] = "lhard;";
static const char lharuSemicolonEntityName[] = "lharu;";
static const char lharulSemicolonEntityName[] = "lharul;";
static const char lhblkSemicolonEntityName[] = "lhblk;";
static const char ljcySemicolonEntityName[] = "ljcy;";
static const char llSemicolonEntityName[] = "ll;";
static const char llarrSemicolonEntityName[] = "llarr;";
static const char llcornerSemicolonEntityName[] = "llcorner;";
static const char llhardSemicolonEntityName[] = "llhard;";
static const char lltriSemicolonEntityName[] = "lltri;";
static const char lmidotSemicolonEntityName[] = "lmidot;";
static const char lmoustSemicolonEntityName[] = "lmoust;";
static const char lmoustacheSemicolonEntityName[] = "lmoustache;";
static const char lnESemicolonEntityName[] = "lnE;";
static const char lnapSemicolonEntityName[] = "lnap;";
static const char lnapproxSemicolonEntityName[] = "lnapprox;";
static const char lneSemicolonEntityName[] = "lne;";
static const char lneqSemicolonEntityName[] = "lneq;";
static const char lneqqSemicolonEntityName[] = "lneqq;";
static const char lnsimSemicolonEntityName[] = "lnsim;";
static const char loangSemicolonEntityName[] = "loang;";
static const char loarrSemicolonEntityName[] = "loarr;";
static const char lobrkSemicolonEntityName[] = "lobrk;";
static const char longleftarrowSemicolonEntityName[] = "longleftarrow;";
static const char longleftrightarrowSemicolonEntityName[] = "longleftrightarrow;";
static const char longmapstoSemicolonEntityName[] = "longmapsto;";
static const char longrightarrowSemicolonEntityName[] = "longrightarrow;";
static const char looparrowleftSemicolonEntityName[] = "looparrowleft;";
static const char looparrowrightSemicolonEntityName[] = "looparrowright;";
static const char loparSemicolonEntityName[] = "lopar;";
static const char lopfSemicolonEntityName[] = "lopf;";
static const char loplusSemicolonEntityName[] = "loplus;";
static const char lotimesSemicolonEntityName[] = "lotimes;";
static const char lowastSemicolonEntityName[] = "lowast;";
static const char lowbarSemicolonEntityName[] = "lowbar;";
static const char lozSemicolonEntityName[] = "loz;";
static const char lozengeSemicolonEntityName[] = "lozenge;";
static const char lozfSemicolonEntityName[] = "lozf;";
static const char lparSemicolonEntityName[] = "lpar;";
static const char lparltSemicolonEntityName[] = "lparlt;";
static const char lrarrSemicolonEntityName[] = "lrarr;";
static const char lrcornerSemicolonEntityName[] = "lrcorner;";
static const char lrharSemicolonEntityName[] = "lrhar;";
static const char lrhardSemicolonEntityName[] = "lrhard;";
static const char lrmSemicolonEntityName[] = "lrm;";
static const char lrtriSemicolonEntityName[] = "lrtri;";
static const char lsaquoSemicolonEntityName[] = "lsaquo;";
static const char lscrSemicolonEntityName[] = "lscr;";
static const char lshSemicolonEntityName[] = "lsh;";
static const char lsimSemicolonEntityName[] = "lsim;";
static const char lsimeSemicolonEntityName[] = "lsime;";
static const char lsimgSemicolonEntityName[] = "lsimg;";
static const char lsqbSemicolonEntityName[] = "lsqb;";
static const char lsquoSemicolonEntityName[] = "lsquo;";
static const char lsquorSemicolonEntityName[] = "lsquor;";
static const char lstrokSemicolonEntityName[] = "lstrok;";
static const char ltEntityName[] = "lt";
static const char ltSemicolonEntityName[] = "lt;";
static const char ltccSemicolonEntityName[] = "ltcc;";
static const char ltcirSemicolonEntityName[] = "ltcir;";
static const char ltdotSemicolonEntityName[] = "ltdot;";
static const char lthreeSemicolonEntityName[] = "lthree;";
static const char ltimesSemicolonEntityName[] = "ltimes;";
static const char ltlarrSemicolonEntityName[] = "ltlarr;";
static const char ltquestSemicolonEntityName[] = "ltquest;";
static const char ltrParSemicolonEntityName[] = "ltrPar;";
static const char ltriSemicolonEntityName[] = "ltri;";
static const char ltrieSemicolonEntityName[] = "ltrie;";
static const char ltrifSemicolonEntityName[] = "ltrif;";
static const char lurdsharSemicolonEntityName[] = "lurdshar;";
static const char luruharSemicolonEntityName[] = "luruhar;";
static const char lvertneqqSemicolonEntityName[] = "lvertneqq;";
static const char lvnESemicolonEntityName[] = "lvnE;";
static const char mDDotSemicolonEntityName[] = "mDDot;";
static const char macrEntityName[] = "macr";
static const char macrSemicolonEntityName[] = "macr;";
static const char maleSemicolonEntityName[] = "male;";
static const char maltSemicolonEntityName[] = "malt;";
static const char malteseSemicolonEntityName[] = "maltese;";
static const char mapSemicolonEntityName[] = "map;";
static const char mapstoSemicolonEntityName[] = "mapsto;";
static const char mapstodownSemicolonEntityName[] = "mapstodown;";
static const char mapstoleftSemicolonEntityName[] = "mapstoleft;";
static const char mapstoupSemicolonEntityName[] = "mapstoup;";
static const char markerSemicolonEntityName[] = "marker;";
static const char mcommaSemicolonEntityName[] = "mcomma;";
static const char mcySemicolonEntityName[] = "mcy;";
static const char mdashSemicolonEntityName[] = "mdash;";
static const char measuredangleSemicolonEntityName[] = "measuredangle;";
static const char mfrSemicolonEntityName[] = "mfr;";
static const char mhoSemicolonEntityName[] = "mho;";
static const char microEntityName[] = "micro";
static const char microSemicolonEntityName[] = "micro;";
static const char midSemicolonEntityName[] = "mid;";
static const char midastSemicolonEntityName[] = "midast;";
static const char midcirSemicolonEntityName[] = "midcir;";
static const char middotEntityName[] = "middot";
static const char middotSemicolonEntityName[] = "middot;";
static const char minusSemicolonEntityName[] = "minus;";
static const char minusbSemicolonEntityName[] = "minusb;";
static const char minusdSemicolonEntityName[] = "minusd;";
static const char minusduSemicolonEntityName[] = "minusdu;";
static const char mlcpSemicolonEntityName[] = "mlcp;";
static const char mldrSemicolonEntityName[] = "mldr;";
static const char mnplusSemicolonEntityName[] = "mnplus;";
static const char modelsSemicolonEntityName[] = "models;";
static const char mopfSemicolonEntityName[] = "mopf;";
static const char mpSemicolonEntityName[] = "mp;";
static const char mscrSemicolonEntityName[] = "mscr;";
static const char mstposSemicolonEntityName[] = "mstpos;";
static const char muSemicolonEntityName[] = "mu;";
static const char multimapSemicolonEntityName[] = "multimap;";
static const char mumapSemicolonEntityName[] = "mumap;";
static const char nGgSemicolonEntityName[] = "nGg;";
static const char nGtSemicolonEntityName[] = "nGt;";
static const char nGtvSemicolonEntityName[] = "nGtv;";
static const char nLeftarrowSemicolonEntityName[] = "nLeftarrow;";
static const char nLeftrightarrowSemicolonEntityName[] = "nLeftrightarrow;";
static const char nLlSemicolonEntityName[] = "nLl;";
static const char nLtSemicolonEntityName[] = "nLt;";
static const char nLtvSemicolonEntityName[] = "nLtv;";
static const char nRightarrowSemicolonEntityName[] = "nRightarrow;";
static const char nVDashSemicolonEntityName[] = "nVDash;";
static const char nVdashSemicolonEntityName[] = "nVdash;";
static const char nablaSemicolonEntityName[] = "nabla;";
static const char nacuteSemicolonEntityName[] = "nacute;";
static const char nangSemicolonEntityName[] = "nang;";
static const char napSemicolonEntityName[] = "nap;";
static const char napESemicolonEntityName[] = "napE;";
static const char napidSemicolonEntityName[] = "napid;";
static const char naposSemicolonEntityName[] = "napos;";
static const char napproxSemicolonEntityName[] = "napprox;";
static const char naturSemicolonEntityName[] = "natur;";
static const char naturalSemicolonEntityName[] = "natural;";
static const char naturalsSemicolonEntityName[] = "naturals;";
static const char nbspEntityName[] = "nbsp";
static const char nbspSemicolonEntityName[] = "nbsp;";
static const char nbumpSemicolonEntityName[] = "nbump;";
static const char nbumpeSemicolonEntityName[] = "nbumpe;";
static const char ncapSemicolonEntityName[] = "ncap;";
static const char ncaronSemicolonEntityName[] = "ncaron;";
static const char ncedilSemicolonEntityName[] = "ncedil;";
static const char ncongSemicolonEntityName[] = "ncong;";
static const char ncongdotSemicolonEntityName[] = "ncongdot;";
static const char ncupSemicolonEntityName[] = "ncup;";
static const char ncySemicolonEntityName[] = "ncy;";
static const char ndashSemicolonEntityName[] = "ndash;";
static const char neSemicolonEntityName[] = "ne;";
static const char neArrSemicolonEntityName[] = "neArr;";
static const char nearhkSemicolonEntityName[] = "nearhk;";
static const char nearrSemicolonEntityName[] = "nearr;";
static const char nearrowSemicolonEntityName[] = "nearrow;";
static const char nedotSemicolonEntityName[] = "nedot;";
static const char nequivSemicolonEntityName[] = "nequiv;";
static const char nesearSemicolonEntityName[] = "nesear;";
static const char nesimSemicolonEntityName[] = "nesim;";
static const char nexistSemicolonEntityName[] = "nexist;";
static const char nexistsSemicolonEntityName[] = "nexists;";
static const char nfrSemicolonEntityName[] = "nfr;";
static const char ngESemicolonEntityName[] = "ngE;";
static const char ngeSemicolonEntityName[] = "nge;";
static const char ngeqSemicolonEntityName[] = "ngeq;";
static const char ngeqqSemicolonEntityName[] = "ngeqq;";
static const char ngeqslantSemicolonEntityName[] = "ngeqslant;";
static const char ngesSemicolonEntityName[] = "nges;";
static const char ngsimSemicolonEntityName[] = "ngsim;";
static const char ngtSemicolonEntityName[] = "ngt;";
static const char ngtrSemicolonEntityName[] = "ngtr;";
static const char nhArrSemicolonEntityName[] = "nhArr;";
static const char nharrSemicolonEntityName[] = "nharr;";
static const char nhparSemicolonEntityName[] = "nhpar;";
static const char niSemicolonEntityName[] = "ni;";
static const char nisSemicolonEntityName[] = "nis;";
static const char nisdSemicolonEntityName[] = "nisd;";
static const char nivSemicolonEntityName[] = "niv;";
static const char njcySemicolonEntityName[] = "njcy;";
static const char nlArrSemicolonEntityName[] = "nlArr;";
static const char nlESemicolonEntityName[] = "nlE;";
static const char nlarrSemicolonEntityName[] = "nlarr;";
static const char nldrSemicolonEntityName[] = "nldr;";
static const char nleSemicolonEntityName[] = "nle;";
static const char nleftarrowSemicolonEntityName[] = "nleftarrow;";
static const char nleftrightarrowSemicolonEntityName[] = "nleftrightarrow;";
static const char nleqSemicolonEntityName[] = "nleq;";
static const char nleqqSemicolonEntityName[] = "nleqq;";
static const char nleqslantSemicolonEntityName[] = "nleqslant;";
static const char nlesSemicolonEntityName[] = "nles;";
static const char nlessSemicolonEntityName[] = "nless;";
static const char nlsimSemicolonEntityName[] = "nlsim;";
static const char nltSemicolonEntityName[] = "nlt;";
static const char nltriSemicolonEntityName[] = "nltri;";
static const char nltrieSemicolonEntityName[] = "nltrie;";
static const char nmidSemicolonEntityName[] = "nmid;";
static const char nopfSemicolonEntityName[] = "nopf;";
static const char notEntityName[] = "not";
static const char notSemicolonEntityName[] = "not;";
static const char notinSemicolonEntityName[] = "notin;";
static const char notinESemicolonEntityName[] = "notinE;";
static const char notindotSemicolonEntityName[] = "notindot;";
static const char notinvaSemicolonEntityName[] = "notinva;";
static const char notinvbSemicolonEntityName[] = "notinvb;";
static const char notinvcSemicolonEntityName[] = "notinvc;";
static const char notniSemicolonEntityName[] = "notni;";
static const char notnivaSemicolonEntityName[] = "notniva;";
static const char notnivbSemicolonEntityName[] = "notnivb;";
static const char notnivcSemicolonEntityName[] = "notnivc;";
static const char nparSemicolonEntityName[] = "npar;";
static const char nparallelSemicolonEntityName[] = "nparallel;";
static const char nparslSemicolonEntityName[] = "nparsl;";
static const char npartSemicolonEntityName[] = "npart;";
static const char npolintSemicolonEntityName[] = "npolint;";
static const char nprSemicolonEntityName[] = "npr;";
static const char nprcueSemicolonEntityName[] = "nprcue;";
static const char npreSemicolonEntityName[] = "npre;";
static const char nprecSemicolonEntityName[] = "nprec;";
static const char npreceqSemicolonEntityName[] = "npreceq;";
static const char nrArrSemicolonEntityName[] = "nrArr;";
static const char nrarrSemicolonEntityName[] = "nrarr;";
static const char nrarrcSemicolonEntityName[] = "nrarrc;";
static const char nrarrwSemicolonEntityName[] = "nrarrw;";
static const char nrightarrowSemicolonEntityName[] = "nrightarrow;";
static const char nrtriSemicolonEntityName[] = "nrtri;";
static const char nrtrieSemicolonEntityName[] = "nrtrie;";
static const char nscSemicolonEntityName[] = "nsc;";
static const char nsccueSemicolonEntityName[] = "nsccue;";
static const char nsceSemicolonEntityName[] = "nsce;";
static const char nscrSemicolonEntityName[] = "nscr;";
static const char nshortmidSemicolonEntityName[] = "nshortmid;";
static const char nshortparallelSemicolonEntityName[] = "nshortparallel;";
static const char nsimSemicolonEntityName[] = "nsim;";
static const char nsimeSemicolonEntityName[] = "nsime;";
static const char nsimeqSemicolonEntityName[] = "nsimeq;";
static const char nsmidSemicolonEntityName[] = "nsmid;";
static const char nsparSemicolonEntityName[] = "nspar;";
static const char nsqsubeSemicolonEntityName[] = "nsqsube;";
static const char nsqsupeSemicolonEntityName[] = "nsqsupe;";
static const char nsubSemicolonEntityName[] = "nsub;";
static const char nsubESemicolonEntityName[] = "nsubE;";
static const char nsubeSemicolonEntityName[] = "nsube;";
static const char nsubsetSemicolonEntityName[] = "nsubset;";
static const char nsubseteqSemicolonEntityName[] = "nsubseteq;";
static const char nsubseteqqSemicolonEntityName[] = "nsubseteqq;";
static const char nsuccSemicolonEntityName[] = "nsucc;";
static const char nsucceqSemicolonEntityName[] = "nsucceq;";
static const char nsupSemicolonEntityName[] = "nsup;";
static const char nsupESemicolonEntityName[] = "nsupE;";
static const char nsupeSemicolonEntityName[] = "nsupe;";
static const char nsupsetSemicolonEntityName[] = "nsupset;";
static const char nsupseteqSemicolonEntityName[] = "nsupseteq;";
static const char nsupseteqqSemicolonEntityName[] = "nsupseteqq;";
static const char ntglSemicolonEntityName[] = "ntgl;";
static const char ntildeEntityName[] = "ntilde";
static const char ntildeSemicolonEntityName[] = "ntilde;";
static const char ntlgSemicolonEntityName[] = "ntlg;";
static const char ntriangleleftSemicolonEntityName[] = "ntriangleleft;";
static const char ntrianglelefteqSemicolonEntityName[] = "ntrianglelefteq;";
static const char ntrianglerightSemicolonEntityName[] = "ntriangleright;";
static const char ntrianglerighteqSemicolonEntityName[] = "ntrianglerighteq;";
static const char nuSemicolonEntityName[] = "nu;";
static const char numSemicolonEntityName[] = "num;";
static const char numeroSemicolonEntityName[] = "numero;";
static const char numspSemicolonEntityName[] = "numsp;";
static const char nvDashSemicolonEntityName[] = "nvDash;";
static const char nvHarrSemicolonEntityName[] = "nvHarr;";
static const char nvapSemicolonEntityName[] = "nvap;";
static const char nvdashSemicolonEntityName[] = "nvdash;";
static const char nvgeSemicolonEntityName[] = "nvge;";
static const char nvgtSemicolonEntityName[] = "nvgt;";
static const char nvinfinSemicolonEntityName[] = "nvinfin;";
static const char nvlArrSemicolonEntityName[] = "nvlArr;";
static const char nvleSemicolonEntityName[] = "nvle;";
static const char nvltSemicolonEntityName[] = "nvlt;";
static const char nvltrieSemicolonEntityName[] = "nvltrie;";
static const char nvrArrSemicolonEntityName[] = "nvrArr;";
static const char nvrtrieSemicolonEntityName[] = "nvrtrie;";
static const char nvsimSemicolonEntityName[] = "nvsim;";
static const char nwArrSemicolonEntityName[] = "nwArr;";
static const char nwarhkSemicolonEntityName[] = "nwarhk;";
static const char nwarrSemicolonEntityName[] = "nwarr;";
static const char nwarrowSemicolonEntityName[] = "nwarrow;";
static const char nwnearSemicolonEntityName[] = "nwnear;";
static const char oSSemicolonEntityName[] = "oS;";
static const char oacuteEntityName[] = "oacute";
static const char oacuteSemicolonEntityName[] = "oacute;";
static const char oastSemicolonEntityName[] = "oast;";
static const char ocirSemicolonEntityName[] = "ocir;";
static const char ocircEntityName[] = "ocirc";
static const char ocircSemicolonEntityName[] = "ocirc;";
static const char ocySemicolonEntityName[] = "ocy;";
static const char odashSemicolonEntityName[] = "odash;";
static const char odblacSemicolonEntityName[] = "odblac;";
static const char odivSemicolonEntityName[] = "odiv;";
static const char odotSemicolonEntityName[] = "odot;";
static const char odsoldSemicolonEntityName[] = "odsold;";
static const char oeligSemicolonEntityName[] = "oelig;";
static const char ofcirSemicolonEntityName[] = "ofcir;";
static const char ofrSemicolonEntityName[] = "ofr;";
static const char ogonSemicolonEntityName[] = "ogon;";
static const char ograveEntityName[] = "ograve";
static const char ograveSemicolonEntityName[] = "ograve;";
static const char ogtSemicolonEntityName[] = "ogt;";
static const char ohbarSemicolonEntityName[] = "ohbar;";
static const char ohmSemicolonEntityName[] = "ohm;";
static const char ointSemicolonEntityName[] = "oint;";
static const char olarrSemicolonEntityName[] = "olarr;";
static const char olcirSemicolonEntityName[] = "olcir;";
static const char olcrossSemicolonEntityName[] = "olcross;";
static const char olineSemicolonEntityName[] = "oline;";
static const char oltSemicolonEntityName[] = "olt;";
static const char omacrSemicolonEntityName[] = "omacr;";
static const char omegaSemicolonEntityName[] = "omega;";
static const char omicronSemicolonEntityName[] = "omicron;";
static const char omidSemicolonEntityName[] = "omid;";
static const char ominusSemicolonEntityName[] = "ominus;";
static const char oopfSemicolonEntityName[] = "oopf;";
static const char oparSemicolonEntityName[] = "opar;";
static const char operpSemicolonEntityName[] = "operp;";
static const char oplusSemicolonEntityName[] = "oplus;";
static const char orSemicolonEntityName[] = "or;";
static const char orarrSemicolonEntityName[] = "orarr;";
static const char ordSemicolonEntityName[] = "ord;";
static const char orderSemicolonEntityName[] = "order;";
static const char orderofSemicolonEntityName[] = "orderof;";
static const char ordfEntityName[] = "ordf";
static const char ordfSemicolonEntityName[] = "ordf;";
static const char ordmEntityName[] = "ordm";
static const char ordmSemicolonEntityName[] = "ordm;";
static const char origofSemicolonEntityName[] = "origof;";
static const char ororSemicolonEntityName[] = "oror;";
static const char orslopeSemicolonEntityName[] = "orslope;";
static const char orvSemicolonEntityName[] = "orv;";
static const char oscrSemicolonEntityName[] = "oscr;";
static const char oslashEntityName[] = "oslash";
static const char oslashSemicolonEntityName[] = "oslash;";
static const char osolSemicolonEntityName[] = "osol;";
static const char otildeEntityName[] = "otilde";
static const char otildeSemicolonEntityName[] = "otilde;";
static const char otimesSemicolonEntityName[] = "otimes;";
static const char otimesasSemicolonEntityName[] = "otimesas;";
static const char oumlEntityName[] = "ouml";
static const char oumlSemicolonEntityName[] = "ouml;";
static const char ovbarSemicolonEntityName[] = "ovbar;";
static const char parSemicolonEntityName[] = "par;";
static const char paraEntityName[] = "para";
static const char paraSemicolonEntityName[] = "para;";
static const char parallelSemicolonEntityName[] = "parallel;";
static const char parsimSemicolonEntityName[] = "parsim;";
static const char parslSemicolonEntityName[] = "parsl;";
static const char partSemicolonEntityName[] = "part;";
static const char pcySemicolonEntityName[] = "pcy;";
static const char percntSemicolonEntityName[] = "percnt;";
static const char periodSemicolonEntityName[] = "period;";
static const char permilSemicolonEntityName[] = "permil;";
static const char perpSemicolonEntityName[] = "perp;";
static const char pertenkSemicolonEntityName[] = "pertenk;";
static const char pfrSemicolonEntityName[] = "pfr;";
static const char phiSemicolonEntityName[] = "phi;";
static const char phivSemicolonEntityName[] = "phiv;";
static const char phmmatSemicolonEntityName[] = "phmmat;";
static const char phoneSemicolonEntityName[] = "phone;";
static const char piSemicolonEntityName[] = "pi;";
static const char pitchforkSemicolonEntityName[] = "pitchfork;";
static const char pivSemicolonEntityName[] = "piv;";
static const char planckSemicolonEntityName[] = "planck;";
static const char planckhSemicolonEntityName[] = "planckh;";
static const char plankvSemicolonEntityName[] = "plankv;";
static const char plusSemicolonEntityName[] = "plus;";
static const char plusacirSemicolonEntityName[] = "plusacir;";
static const char plusbSemicolonEntityName[] = "plusb;";
static const char pluscirSemicolonEntityName[] = "pluscir;";
static const char plusdoSemicolonEntityName[] = "plusdo;";
static const char plusduSemicolonEntityName[] = "plusdu;";
static const char pluseSemicolonEntityName[] = "pluse;";
static const char plusmnEntityName[] = "plusmn";
static const char plusmnSemicolonEntityName[] = "plusmn;";
static const char plussimSemicolonEntityName[] = "plussim;";
static const char plustwoSemicolonEntityName[] = "plustwo;";
static const char pmSemicolonEntityName[] = "pm;";
static const char pointintSemicolonEntityName[] = "pointint;";
static const char popfSemicolonEntityName[] = "popf;";
static const char poundEntityName[] = "pound";
static const char poundSemicolonEntityName[] = "pound;";
static const char prSemicolonEntityName[] = "pr;";
static const char prESemicolonEntityName[] = "prE;";
static const char prapSemicolonEntityName[] = "prap;";
static const char prcueSemicolonEntityName[] = "prcue;";
static const char preSemicolonEntityName[] = "pre;";
static const char precSemicolonEntityName[] = "prec;";
static const char precapproxSemicolonEntityName[] = "precapprox;";
static const char preccurlyeqSemicolonEntityName[] = "preccurlyeq;";
static const char preceqSemicolonEntityName[] = "preceq;";
static const char precnapproxSemicolonEntityName[] = "precnapprox;";
static const char precneqqSemicolonEntityName[] = "precneqq;";
static const char precnsimSemicolonEntityName[] = "precnsim;";
static const char precsimSemicolonEntityName[] = "precsim;";
static const char primeSemicolonEntityName[] = "prime;";
static const char primesSemicolonEntityName[] = "primes;";
static const char prnESemicolonEntityName[] = "prnE;";
static const char prnapSemicolonEntityName[] = "prnap;";
static const char prnsimSemicolonEntityName[] = "prnsim;";
static const char prodSemicolonEntityName[] = "prod;";
static const char profalarSemicolonEntityName[] = "profalar;";
static const char proflineSemicolonEntityName[] = "profline;";
static const char profsurfSemicolonEntityName[] = "profsurf;";
static const char propSemicolonEntityName[] = "prop;";
static const char proptoSemicolonEntityName[] = "propto;";
static const char prsimSemicolonEntityName[] = "prsim;";
static const char prurelSemicolonEntityName[] = "prurel;";
static const char pscrSemicolonEntityName[] = "pscr;";
static const char psiSemicolonEntityName[] = "psi;";
static const char puncspSemicolonEntityName[] = "puncsp;";
static const char qfrSemicolonEntityName[] = "qfr;";
static const char qintSemicolonEntityName[] = "qint;";
static const char qopfSemicolonEntityName[] = "qopf;";
static const char qprimeSemicolonEntityName[] = "qprime;";
static const char qscrSemicolonEntityName[] = "qscr;";
static const char quaternionsSemicolonEntityName[] = "quaternions;";
static const char quatintSemicolonEntityName[] = "quatint;";
static const char questSemicolonEntityName[] = "quest;";
static const char questeqSemicolonEntityName[] = "questeq;";
static const char quotEntityName[] = "quot";
static const char quotSemicolonEntityName[] = "quot;";
static const char rAarrSemicolonEntityName[] = "rAarr;";
static const char rArrSemicolonEntityName[] = "rArr;";
static const char rAtailSemicolonEntityName[] = "rAtail;";
static const char rBarrSemicolonEntityName[] = "rBarr;";
static const char rHarSemicolonEntityName[] = "rHar;";
static const char raceSemicolonEntityName[] = "race;";
static const char racuteSemicolonEntityName[] = "racute;";
static const char radicSemicolonEntityName[] = "radic;";
static const char raemptyvSemicolonEntityName[] = "raemptyv;";
static const char rangSemicolonEntityName[] = "rang;";
static const char rangdSemicolonEntityName[] = "rangd;";
static const char rangeSemicolonEntityName[] = "range;";
static const char rangleSemicolonEntityName[] = "rangle;";
static const char raquoEntityName[] = "raquo";
static const char raquoSemicolonEntityName[] = "raquo;";
static const char rarrSemicolonEntityName[] = "rarr;";
static const char rarrapSemicolonEntityName[] = "rarrap;";
static const char rarrbSemicolonEntityName[] = "rarrb;";
static const char rarrbfsSemicolonEntityName[] = "rarrbfs;";
static const char rarrcSemicolonEntityName[] = "rarrc;";
static const char rarrfsSemicolonEntityName[] = "rarrfs;";
static const char rarrhkSemicolonEntityName[] = "rarrhk;";
static const char rarrlpSemicolonEntityName[] = "rarrlp;";
static const char rarrplSemicolonEntityName[] = "rarrpl;";
static const char rarrsimSemicolonEntityName[] = "rarrsim;";
static const char rarrtlSemicolonEntityName[] = "rarrtl;";
static const char rarrwSemicolonEntityName[] = "rarrw;";
static const char ratailSemicolonEntityName[] = "ratail;";
static const char ratioSemicolonEntityName[] = "ratio;";
static const char rationalsSemicolonEntityName[] = "rationals;";
static const char rbarrSemicolonEntityName[] = "rbarr;";
static const char rbbrkSemicolonEntityName[] = "rbbrk;";
static const char rbraceSemicolonEntityName[] = "rbrace;";
static const char rbrackSemicolonEntityName[] = "rbrack;";
static const char rbrkeSemicolonEntityName[] = "rbrke;";
static const char rbrksldSemicolonEntityName[] = "rbrksld;";
static const char rbrksluSemicolonEntityName[] = "rbrkslu;";
static const char rcaronSemicolonEntityName[] = "rcaron;";
static const char rcedilSemicolonEntityName[] = "rcedil;";
static const char rceilSemicolonEntityName[] = "rceil;";
static const char rcubSemicolonEntityName[] = "rcub;";
static const char rcySemicolonEntityName[] = "rcy;";
static const char rdcaSemicolonEntityName[] = "rdca;";
static const char rdldharSemicolonEntityName[] = "rdldhar;";
static const char rdquoSemicolonEntityName[] = "rdquo;";
static const char rdquorSemicolonEntityName[] = "rdquor;";
static const char rdshSemicolonEntityName[] = "rdsh;";
static const char realSemicolonEntityName[] = "real;";
static const char realineSemicolonEntityName[] = "realine;";
static const char realpartSemicolonEntityName[] = "realpart;";
static const char realsSemicolonEntityName[] = "reals;";
static const char rectSemicolonEntityName[] = "rect;";
static const char regEntityName[] = "reg";
static const char regSemicolonEntityName[] = "reg;";
static const char rfishtSemicolonEntityName[] = "rfisht;";
static const char rfloorSemicolonEntityName[] = "rfloor;";
static const char rfrSemicolonEntityName[] = "rfr;";
static const char rhardSemicolonEntityName[] = "rhard;";
static const char rharuSemicolonEntityName[] = "rharu;";
static const char rharulSemicolonEntityName[] = "rharul;";
static const char rhoSemicolonEntityName[] = "rho;";
static const char rhovSemicolonEntityName[] = "rhov;";
static const char rightarrowSemicolonEntityName[] = "rightarrow;";
static const char rightarrowtailSemicolonEntityName[] = "rightarrowtail;";
static const char rightharpoondownSemicolonEntityName[] = "rightharpoondown;";
static const char rightharpoonupSemicolonEntityName[] = "rightharpoonup;";
static const char rightleftarrowsSemicolonEntityName[] = "rightleftarrows;";
static const char rightleftharpoonsSemicolonEntityName[] = "rightleftharpoons;";
static const char rightrightarrowsSemicolonEntityName[] = "rightrightarrows;";
static const char rightsquigarrowSemicolonEntityName[] = "rightsquigarrow;";
static const char rightthreetimesSemicolonEntityName[] = "rightthreetimes;";
static const char ringSemicolonEntityName[] = "ring;";
static const char risingdotseqSemicolonEntityName[] = "risingdotseq;";
static const char rlarrSemicolonEntityName[] = "rlarr;";
static const char rlharSemicolonEntityName[] = "rlhar;";
static const char rlmSemicolonEntityName[] = "rlm;";
static const char rmoustSemicolonEntityName[] = "rmoust;";
static const char rmoustacheSemicolonEntityName[] = "rmoustache;";
static const char rnmidSemicolonEntityName[] = "rnmid;";
static const char roangSemicolonEntityName[] = "roang;";
static const char roarrSemicolonEntityName[] = "roarr;";
static const char robrkSemicolonEntityName[] = "robrk;";
static const char roparSemicolonEntityName[] = "ropar;";
static const char ropfSemicolonEntityName[] = "ropf;";
static const char roplusSemicolonEntityName[] = "roplus;";
static const char rotimesSemicolonEntityName[] = "rotimes;";
static const char rparSemicolonEntityName[] = "rpar;";
static const char rpargtSemicolonEntityName[] = "rpargt;";
static const char rppolintSemicolonEntityName[] = "rppolint;";
static const char rrarrSemicolonEntityName[] = "rrarr;";
static const char rsaquoSemicolonEntityName[] = "rsaquo;";
static const char rscrSemicolonEntityName[] = "rscr;";
static const char rshSemicolonEntityName[] = "rsh;";
static const char rsqbSemicolonEntityName[] = "rsqb;";
static const char rsquoSemicolonEntityName[] = "rsquo;";
static const char rsquorSemicolonEntityName[] = "rsquor;";
static const char rthreeSemicolonEntityName[] = "rthree;";
static const char rtimesSemicolonEntityName[] = "rtimes;";
static const char rtriSemicolonEntityName[] = "rtri;";
static const char rtrieSemicolonEntityName[] = "rtrie;";
static const char rtrifSemicolonEntityName[] = "rtrif;";
static const char rtriltriSemicolonEntityName[] = "rtriltri;";
static const char ruluharSemicolonEntityName[] = "ruluhar;";
static const char rxSemicolonEntityName[] = "rx;";
static const char sacuteSemicolonEntityName[] = "sacute;";
static const char sbquoSemicolonEntityName[] = "sbquo;";
static const char scSemicolonEntityName[] = "sc;";
static const char scESemicolonEntityName[] = "scE;";
static const char scapSemicolonEntityName[] = "scap;";
static const char scaronSemicolonEntityName[] = "scaron;";
static const char sccueSemicolonEntityName[] = "sccue;";
static const char sceSemicolonEntityName[] = "sce;";
static const char scedilSemicolonEntityName[] = "scedil;";
static const char scircSemicolonEntityName[] = "scirc;";
static const char scnESemicolonEntityName[] = "scnE;";
static const char scnapSemicolonEntityName[] = "scnap;";
static const char scnsimSemicolonEntityName[] = "scnsim;";
static const char scpolintSemicolonEntityName[] = "scpolint;";
static const char scsimSemicolonEntityName[] = "scsim;";
static const char scySemicolonEntityName[] = "scy;";
static const char sdotSemicolonEntityName[] = "sdot;";
static const char sdotbSemicolonEntityName[] = "sdotb;";
static const char sdoteSemicolonEntityName[] = "sdote;";
static const char seArrSemicolonEntityName[] = "seArr;";
static const char searhkSemicolonEntityName[] = "searhk;";
static const char searrSemicolonEntityName[] = "searr;";
static const char searrowSemicolonEntityName[] = "searrow;";
static const char sectEntityName[] = "sect";
static const char sectSemicolonEntityName[] = "sect;";
static const char semiSemicolonEntityName[] = "semi;";
static const char seswarSemicolonEntityName[] = "seswar;";
static const char setminusSemicolonEntityName[] = "setminus;";
static const char setmnSemicolonEntityName[] = "setmn;";
static const char sextSemicolonEntityName[] = "sext;";
static const char sfrSemicolonEntityName[] = "sfr;";
static const char sfrownSemicolonEntityName[] = "sfrown;";
static const char sharpSemicolonEntityName[] = "sharp;";
static const char shchcySemicolonEntityName[] = "shchcy;";
static const char shcySemicolonEntityName[] = "shcy;";
static const char shortmidSemicolonEntityName[] = "shortmid;";
static const char shortparallelSemicolonEntityName[] = "shortparallel;";
static const char shyEntityName[] = "shy";
static const char shySemicolonEntityName[] = "shy;";
static const char sigmaSemicolonEntityName[] = "sigma;";
static const char sigmafSemicolonEntityName[] = "sigmaf;";
static const char sigmavSemicolonEntityName[] = "sigmav;";
static const char simSemicolonEntityName[] = "sim;";
static const char simdotSemicolonEntityName[] = "simdot;";
static const char simeSemicolonEntityName[] = "sime;";
static const char simeqSemicolonEntityName[] = "simeq;";
static const char simgSemicolonEntityName[] = "simg;";
static const char simgESemicolonEntityName[] = "simgE;";
static const char simlSemicolonEntityName[] = "siml;";
static const char simlESemicolonEntityName[] = "simlE;";
static const char simneSemicolonEntityName[] = "simne;";
static const char simplusSemicolonEntityName[] = "simplus;";
static const char simrarrSemicolonEntityName[] = "simrarr;";
static const char slarrSemicolonEntityName[] = "slarr;";
static const char smallsetminusSemicolonEntityName[] = "smallsetminus;";
static const char smashpSemicolonEntityName[] = "smashp;";
static const char smeparslSemicolonEntityName[] = "smeparsl;";
static const char smidSemicolonEntityName[] = "smid;";
static const char smileSemicolonEntityName[] = "smile;";
static const char smtSemicolonEntityName[] = "smt;";
static const char smteSemicolonEntityName[] = "smte;";
static const char smtesSemicolonEntityName[] = "smtes;";
static const char softcySemicolonEntityName[] = "softcy;";
static const char solSemicolonEntityName[] = "sol;";
static const char solbSemicolonEntityName[] = "solb;";
static const char solbarSemicolonEntityName[] = "solbar;";
static const char sopfSemicolonEntityName[] = "sopf;";
static const char spadesSemicolonEntityName[] = "spades;";
static const char spadesuitSemicolonEntityName[] = "spadesuit;";
static const char sparSemicolonEntityName[] = "spar;";
static const char sqcapSemicolonEntityName[] = "sqcap;";
static const char sqcapsSemicolonEntityName[] = "sqcaps;";
static const char sqcupSemicolonEntityName[] = "sqcup;";
static const char sqcupsSemicolonEntityName[] = "sqcups;";
static const char sqsubSemicolonEntityName[] = "sqsub;";
static const char sqsubeSemicolonEntityName[] = "sqsube;";
static const char sqsubsetSemicolonEntityName[] = "sqsubset;";
static const char sqsubseteqSemicolonEntityName[] = "sqsubseteq;";
static const char sqsupSemicolonEntityName[] = "sqsup;";
static const char sqsupeSemicolonEntityName[] = "sqsupe;";
static const char sqsupsetSemicolonEntityName[] = "sqsupset;";
static const char sqsupseteqSemicolonEntityName[] = "sqsupseteq;";
static const char squSemicolonEntityName[] = "squ;";
static const char squareSemicolonEntityName[] = "square;";
static const char squarfSemicolonEntityName[] = "squarf;";
static const char squfSemicolonEntityName[] = "squf;";
static const char srarrSemicolonEntityName[] = "srarr;";
static const char sscrSemicolonEntityName[] = "sscr;";
static const char ssetmnSemicolonEntityName[] = "ssetmn;";
static const char ssmileSemicolonEntityName[] = "ssmile;";
static const char sstarfSemicolonEntityName[] = "sstarf;";
static const char starSemicolonEntityName[] = "star;";
static const char starfSemicolonEntityName[] = "starf;";
static const char straightepsilonSemicolonEntityName[] = "straightepsilon;";
static const char straightphiSemicolonEntityName[] = "straightphi;";
static const char strnsSemicolonEntityName[] = "strns;";
static const char subSemicolonEntityName[] = "sub;";
static const char subESemicolonEntityName[] = "subE;";
static const char subdotSemicolonEntityName[] = "subdot;";
static const char subeSemicolonEntityName[] = "sube;";
static const char subedotSemicolonEntityName[] = "subedot;";
static const char submultSemicolonEntityName[] = "submult;";
static const char subnESemicolonEntityName[] = "subnE;";
static const char subneSemicolonEntityName[] = "subne;";
static const char subplusSemicolonEntityName[] = "subplus;";
static const char subrarrSemicolonEntityName[] = "subrarr;";
static const char subsetSemicolonEntityName[] = "subset;";
static const char subseteqSemicolonEntityName[] = "subseteq;";
static const char subseteqqSemicolonEntityName[] = "subseteqq;";
static const char subsetneqSemicolonEntityName[] = "subsetneq;";
static const char subsetneqqSemicolonEntityName[] = "subsetneqq;";
static const char subsimSemicolonEntityName[] = "subsim;";
static const char subsubSemicolonEntityName[] = "subsub;";
static const char subsupSemicolonEntityName[] = "subsup;";
static const char succSemicolonEntityName[] = "succ;";
static const char succapproxSemicolonEntityName[] = "succapprox;";
static const char succcurlyeqSemicolonEntityName[] = "succcurlyeq;";
static const char succeqSemicolonEntityName[] = "succeq;";
static const char succnapproxSemicolonEntityName[] = "succnapprox;";
static const char succneqqSemicolonEntityName[] = "succneqq;";
static const char succnsimSemicolonEntityName[] = "succnsim;";
static const char succsimSemicolonEntityName[] = "succsim;";
static const char sumSemicolonEntityName[] = "sum;";
static const char sungSemicolonEntityName[] = "sung;";
static const char sup1EntityName[] = "sup1";
static const char sup1SemicolonEntityName[] = "sup1;";
static const char sup2EntityName[] = "sup2";
static const char sup2SemicolonEntityName[] = "sup2;";
static const char sup3EntityName[] = "sup3";
static const char sup3SemicolonEntityName[] = "sup3;";
static const char supSemicolonEntityName[] = "sup;";
static const char supESemicolonEntityName[] = "supE;";
static const char supdotSemicolonEntityName[] = "supdot;";
static const char supdsubSemicolonEntityName[] = "supdsub;";
static const char supeSemicolonEntityName[] = "supe;";
static const char supedotSemicolonEntityName[] = "supedot;";
static const char suphsolSemicolonEntityName[] = "suphsol;";
static const char suphsubSemicolonEntityName[] = "suphsub;";
static const char suplarrSemicolonEntityName[] = "suplarr;";
static const char supmultSemicolonEntityName[] = "supmult;";
static const char supnESemicolonEntityName[] = "supnE;";
static const char supneSemicolonEntityName[] = "supne;";
static const char supplusSemicolonEntityName[] = "supplus;";
static const char supsetSemicolonEntityName[] = "supset;";
static const char supseteqSemicolonEntityName[] = "supseteq;";
static const char supseteqqSemicolonEntityName[] = "supseteqq;";
static const char supsetneqSemicolonEntityName[] = "supsetneq;";
static const char supsetneqqSemicolonEntityName[] = "supsetneqq;";
static const char supsimSemicolonEntityName[] = "supsim;";
static const char supsubSemicolonEntityName[] = "supsub;";
static const char supsupSemicolonEntityName[] = "supsup;";
static const char swArrSemicolonEntityName[] = "swArr;";
static const char swarhkSemicolonEntityName[] = "swarhk;";
static const char swarrSemicolonEntityName[] = "swarr;";
static const char swarrowSemicolonEntityName[] = "swarrow;";
static const char swnwarSemicolonEntityName[] = "swnwar;";
static const char szligEntityName[] = "szlig";
static const char szligSemicolonEntityName[] = "szlig;";
static const char targetSemicolonEntityName[] = "target;";
static const char tauSemicolonEntityName[] = "tau;";
static const char tbrkSemicolonEntityName[] = "tbrk;";
static const char tcaronSemicolonEntityName[] = "tcaron;";
static const char tcedilSemicolonEntityName[] = "tcedil;";
static const char tcySemicolonEntityName[] = "tcy;";
static const char tdotSemicolonEntityName[] = "tdot;";
static const char telrecSemicolonEntityName[] = "telrec;";
static const char tfrSemicolonEntityName[] = "tfr;";
static const char there4SemicolonEntityName[] = "there4;";
static const char thereforeSemicolonEntityName[] = "therefore;";
static const char thetaSemicolonEntityName[] = "theta;";
static const char thetasymSemicolonEntityName[] = "thetasym;";
static const char thetavSemicolonEntityName[] = "thetav;";
static const char thickapproxSemicolonEntityName[] = "thickapprox;";
static const char thicksimSemicolonEntityName[] = "thicksim;";
static const char thinspSemicolonEntityName[] = "thinsp;";
static const char thkapSemicolonEntityName[] = "thkap;";
static const char thksimSemicolonEntityName[] = "thksim;";
static const char thornEntityName[] = "thorn";
static const char thornSemicolonEntityName[] = "thorn;";
static const char tildeSemicolonEntityName[] = "tilde;";
static const char timesEntityName[] = "times";
static const char timesSemicolonEntityName[] = "times;";
static const char timesbSemicolonEntityName[] = "timesb;";
static const char timesbarSemicolonEntityName[] = "timesbar;";
static const char timesdSemicolonEntityName[] = "timesd;";
static const char tintSemicolonEntityName[] = "tint;";
static const char toeaSemicolonEntityName[] = "toea;";
static const char topSemicolonEntityName[] = "top;";
static const char topbotSemicolonEntityName[] = "topbot;";
static const char topcirSemicolonEntityName[] = "topcir;";
static const char topfSemicolonEntityName[] = "topf;";
static const char topforkSemicolonEntityName[] = "topfork;";
static const char tosaSemicolonEntityName[] = "tosa;";
static const char tprimeSemicolonEntityName[] = "tprime;";
static const char tradeSemicolonEntityName[] = "trade;";
static const char triangleSemicolonEntityName[] = "triangle;";
static const char triangledownSemicolonEntityName[] = "triangledown;";
static const char triangleleftSemicolonEntityName[] = "triangleleft;";
static const char trianglelefteqSemicolonEntityName[] = "trianglelefteq;";
static const char triangleqSemicolonEntityName[] = "triangleq;";
static const char trianglerightSemicolonEntityName[] = "triangleright;";
static const char trianglerighteqSemicolonEntityName[] = "trianglerighteq;";
static const char tridotSemicolonEntityName[] = "tridot;";
static const char trieSemicolonEntityName[] = "trie;";
static const char triminusSemicolonEntityName[] = "triminus;";
static const char triplusSemicolonEntityName[] = "triplus;";
static const char trisbSemicolonEntityName[] = "trisb;";
static const char tritimeSemicolonEntityName[] = "tritime;";
static const char trpeziumSemicolonEntityName[] = "trpezium;";
static const char tscrSemicolonEntityName[] = "tscr;";
static const char tscySemicolonEntityName[] = "tscy;";
static const char tshcySemicolonEntityName[] = "tshcy;";
static const char tstrokSemicolonEntityName[] = "tstrok;";
static const char twixtSemicolonEntityName[] = "twixt;";
static const char twoheadleftarrowSemicolonEntityName[] = "twoheadleftarrow;";
static const char twoheadrightarrowSemicolonEntityName[] = "twoheadrightarrow;";
static const char uArrSemicolonEntityName[] = "uArr;";
static const char uHarSemicolonEntityName[] = "uHar;";
static const char uacuteEntityName[] = "uacute";
static const char uacuteSemicolonEntityName[] = "uacute;";
static const char uarrSemicolonEntityName[] = "uarr;";
static const char ubrcySemicolonEntityName[] = "ubrcy;";
static const char ubreveSemicolonEntityName[] = "ubreve;";
static const char ucircEntityName[] = "ucirc";
static const char ucircSemicolonEntityName[] = "ucirc;";
static const char ucySemicolonEntityName[] = "ucy;";
static const char udarrSemicolonEntityName[] = "udarr;";
static const char udblacSemicolonEntityName[] = "udblac;";
static const char udharSemicolonEntityName[] = "udhar;";
static const char ufishtSemicolonEntityName[] = "ufisht;";
static const char ufrSemicolonEntityName[] = "ufr;";
static const char ugraveEntityName[] = "ugrave";
static const char ugraveSemicolonEntityName[] = "ugrave;";
static const char uharlSemicolonEntityName[] = "uharl;";
static const char uharrSemicolonEntityName[] = "uharr;";
static const char uhblkSemicolonEntityName[] = "uhblk;";
static const char ulcornSemicolonEntityName[] = "ulcorn;";
static const char ulcornerSemicolonEntityName[] = "ulcorner;";
static const char ulcropSemicolonEntityName[] = "ulcrop;";
static const char ultriSemicolonEntityName[] = "ultri;";
static const char umacrSemicolonEntityName[] = "umacr;";
static const char umlEntityName[] = "uml";
static const char umlSemicolonEntityName[] = "uml;";
static const char uogonSemicolonEntityName[] = "uogon;";
static const char uopfSemicolonEntityName[] = "uopf;";
static const char uparrowSemicolonEntityName[] = "uparrow;";
static const char updownarrowSemicolonEntityName[] = "updownarrow;";
static const char upharpoonleftSemicolonEntityName[] = "upharpoonleft;";
static const char upharpoonrightSemicolonEntityName[] = "upharpoonright;";
static const char uplusSemicolonEntityName[] = "uplus;";
static const char upsiSemicolonEntityName[] = "upsi;";
static const char upsihSemicolonEntityName[] = "upsih;";
static const char upsilonSemicolonEntityName[] = "upsilon;";
static const char upuparrowsSemicolonEntityName[] = "upuparrows;";
static const char urcornSemicolonEntityName[] = "urcorn;";
static const char urcornerSemicolonEntityName[] = "urcorner;";
static const char urcropSemicolonEntityName[] = "urcrop;";
static const char uringSemicolonEntityName[] = "uring;";
static const char urtriSemicolonEntityName[] = "urtri;";
static const char uscrSemicolonEntityName[] = "uscr;";
static const char utdotSemicolonEntityName[] = "utdot;";
static const char utildeSemicolonEntityName[] = "utilde;";
static const char utriSemicolonEntityName[] = "utri;";
static const char utrifSemicolonEntityName[] = "utrif;";
static const char uuarrSemicolonEntityName[] = "uuarr;";
static const char uumlEntityName[] = "uuml";
static const char uumlSemicolonEntityName[] = "uuml;";
static const char uwangleSemicolonEntityName[] = "uwangle;";
static const char vArrSemicolonEntityName[] = "vArr;";
static const char vBarSemicolonEntityName[] = "vBar;";
static const char vBarvSemicolonEntityName[] = "vBarv;";
static const char vDashSemicolonEntityName[] = "vDash;";
static const char vangrtSemicolonEntityName[] = "vangrt;";
static const char varepsilonSemicolonEntityName[] = "varepsilon;";
static const char varkappaSemicolonEntityName[] = "varkappa;";
static const char varnothingSemicolonEntityName[] = "varnothing;";
static const char varphiSemicolonEntityName[] = "varphi;";
static const char varpiSemicolonEntityName[] = "varpi;";
static const char varproptoSemicolonEntityName[] = "varpropto;";
static const char varrSemicolonEntityName[] = "varr;";
static const char varrhoSemicolonEntityName[] = "varrho;";
static const char varsigmaSemicolonEntityName[] = "varsigma;";
static const char varsubsetneqSemicolonEntityName[] = "varsubsetneq;";
static const char varsubsetneqqSemicolonEntityName[] = "varsubsetneqq;";
static const char varsupsetneqSemicolonEntityName[] = "varsupsetneq;";
static const char varsupsetneqqSemicolonEntityName[] = "varsupsetneqq;";
static const char varthetaSemicolonEntityName[] = "vartheta;";
static const char vartriangleleftSemicolonEntityName[] = "vartriangleleft;";
static const char vartrianglerightSemicolonEntityName[] = "vartriangleright;";
static const char vcySemicolonEntityName[] = "vcy;";
static const char vdashSemicolonEntityName[] = "vdash;";
static const char veeSemicolonEntityName[] = "vee;";
static const char veebarSemicolonEntityName[] = "veebar;";
static const char veeeqSemicolonEntityName[] = "veeeq;";
static const char vellipSemicolonEntityName[] = "vellip;";
static const char verbarSemicolonEntityName[] = "verbar;";
static const char vertSemicolonEntityName[] = "vert;";
static const char vfrSemicolonEntityName[] = "vfr;";
static const char vltriSemicolonEntityName[] = "vltri;";
static const char vnsubSemicolonEntityName[] = "vnsub;";
static const char vnsupSemicolonEntityName[] = "vnsup;";
static const char vopfSemicolonEntityName[] = "vopf;";
static const char vpropSemicolonEntityName[] = "vprop;";
static const char vrtriSemicolonEntityName[] = "vrtri;";
static const char vscrSemicolonEntityName[] = "vscr;";
static const char vsubnESemicolonEntityName[] = "vsubnE;";
static const char vsubneSemicolonEntityName[] = "vsubne;";
static const char vsupnESemicolonEntityName[] = "vsupnE;";
static const char vsupneSemicolonEntityName[] = "vsupne;";
static const char vzigzagSemicolonEntityName[] = "vzigzag;";
static const char wcircSemicolonEntityName[] = "wcirc;";
static const char wedbarSemicolonEntityName[] = "wedbar;";
static const char wedgeSemicolonEntityName[] = "wedge;";
static const char wedgeqSemicolonEntityName[] = "wedgeq;";
static const char weierpSemicolonEntityName[] = "weierp;";
static const char wfrSemicolonEntityName[] = "wfr;";
static const char wopfSemicolonEntityName[] = "wopf;";
static const char wpSemicolonEntityName[] = "wp;";
static const char wrSemicolonEntityName[] = "wr;";
static const char wreathSemicolonEntityName[] = "wreath;";
static const char wscrSemicolonEntityName[] = "wscr;";
static const char xcapSemicolonEntityName[] = "xcap;";
static const char xcircSemicolonEntityName[] = "xcirc;";
static const char xcupSemicolonEntityName[] = "xcup;";
static const char xdtriSemicolonEntityName[] = "xdtri;";
static const char xfrSemicolonEntityName[] = "xfr;";
static const char xhArrSemicolonEntityName[] = "xhArr;";
static const char xharrSemicolonEntityName[] = "xharr;";
static const char xiSemicolonEntityName[] = "xi;";
static const char xlArrSemicolonEntityName[] = "xlArr;";
static const char xlarrSemicolonEntityName[] = "xlarr;";
static const char xmapSemicolonEntityName[] = "xmap;";
static const char xnisSemicolonEntityName[] = "xnis;";
static const char xodotSemicolonEntityName[] = "xodot;";
static const char xopfSemicolonEntityName[] = "xopf;";
static const char xoplusSemicolonEntityName[] = "xoplus;";
static const char xotimeSemicolonEntityName[] = "xotime;";
static const char xrArrSemicolonEntityName[] = "xrArr;";
static const char xrarrSemicolonEntityName[] = "xrarr;";
static const char xscrSemicolonEntityName[] = "xscr;";
static const char xsqcupSemicolonEntityName[] = "xsqcup;";
static const char xuplusSemicolonEntityName[] = "xuplus;";
static const char xutriSemicolonEntityName[] = "xutri;";
static const char xveeSemicolonEntityName[] = "xvee;";
static const char xwedgeSemicolonEntityName[] = "xwedge;";
static const char yacuteEntityName[] = "yacute";
static const char yacuteSemicolonEntityName[] = "yacute;";
static const char yacySemicolonEntityName[] = "yacy;";
static const char ycircSemicolonEntityName[] = "ycirc;";
static const char ycySemicolonEntityName[] = "ycy;";
static const char yenEntityName[] = "yen";
static const char yenSemicolonEntityName[] = "yen;";
static const char yfrSemicolonEntityName[] = "yfr;";
static const char yicySemicolonEntityName[] = "yicy;";
static const char yopfSemicolonEntityName[] = "yopf;";
static const char yscrSemicolonEntityName[] = "yscr;";
static const char yucySemicolonEntityName[] = "yucy;";
static const char yumlEntityName[] = "yuml";
static const char yumlSemicolonEntityName[] = "yuml;";
static const char zacuteSemicolonEntityName[] = "zacute;";
static const char zcaronSemicolonEntityName[] = "zcaron;";
static const char zcySemicolonEntityName[] = "zcy;";
static const char zdotSemicolonEntityName[] = "zdot;";
static const char zeetrfSemicolonEntityName[] = "zeetrf;";
static const char zetaSemicolonEntityName[] = "zeta;";
static const char zfrSemicolonEntityName[] = "zfr;";
static const char zhcySemicolonEntityName[] = "zhcy;";
static const char zigrarrSemicolonEntityName[] = "zigrarr;";
static const char zopfSemicolonEntityName[] = "zopf;";
static const char zscrSemicolonEntityName[] = "zscr;";
static const char zwjSemicolonEntityName[] = "zwj;";
static const char zwnjSemicolonEntityName[] = "zwnj;";

static const wchar_t AEligEntityValue[] = { 0x000C6 };
static const wchar_t AEligSemicolonEntityValue[] = { 0x000C6 };
static const wchar_t AMPEntityValue[] = { 0x00026 };
static const wchar_t AMPSemicolonEntityValue[] = { 0x00026 };
static const wchar_t AacuteEntityValue[] = { 0x000C1 };
static const wchar_t AacuteSemicolonEntityValue[] = { 0x000C1 };
static const wchar_t AbreveSemicolonEntityValue[] = { 0x00102 };
static const wchar_t AcircEntityValue[] = { 0x000C2 };
static const wchar_t AcircSemicolonEntityValue[] = { 0x000C2 };
static const wchar_t AcySemicolonEntityValue[] = { 0x00410 };
static const wchar_t AfrSemicolonEntityValue[] = { 0x1D504 };
static const wchar_t AgraveEntityValue[] = { 0x000C0 };
static const wchar_t AgraveSemicolonEntityValue[] = { 0x000C0 };
static const wchar_t AlphaSemicolonEntityValue[] = { 0x00391 };
static const wchar_t AmacrSemicolonEntityValue[] = { 0x00100 };
static const wchar_t AndSemicolonEntityValue[] = { 0x02A53 };
static const wchar_t AogonSemicolonEntityValue[] = { 0x00104 };
static const wchar_t AopfSemicolonEntityValue[] = { 0x1D538 };
static const wchar_t ApplyFunctionSemicolonEntityValue[] = { 0x02061 };
static const wchar_t AringEntityValue[] = { 0x000C5 };
static const wchar_t AringSemicolonEntityValue[] = { 0x000C5 };
static const wchar_t AscrSemicolonEntityValue[] = { 0x1D49C };
static const wchar_t AssignSemicolonEntityValue[] = { 0x02254 };
static const wchar_t AtildeEntityValue[] = { 0x000C3 };
static const wchar_t AtildeSemicolonEntityValue[] = { 0x000C3 };
static const wchar_t AumlEntityValue[] = { 0x000C4 };
static const wchar_t AumlSemicolonEntityValue[] = { 0x000C4 };
static const wchar_t BackslashSemicolonEntityValue[] = { 0x02216 };
static const wchar_t BarvSemicolonEntityValue[] = { 0x02AE7 };
static const wchar_t BarwedSemicolonEntityValue[] = { 0x02306 };
static const wchar_t BcySemicolonEntityValue[] = { 0x00411 };
static const wchar_t BecauseSemicolonEntityValue[] = { 0x02235 };
static const wchar_t BernoullisSemicolonEntityValue[] = { 0x0212C };
static const wchar_t BetaSemicolonEntityValue[] = { 0x00392 };
static const wchar_t BfrSemicolonEntityValue[] = { 0x1D505 };
static const wchar_t BopfSemicolonEntityValue[] = { 0x1D539 };
static const wchar_t BreveSemicolonEntityValue[] = { 0x002D8 };
static const wchar_t BscrSemicolonEntityValue[] = { 0x0212C };
static const wchar_t BumpeqSemicolonEntityValue[] = { 0x0224E };
static const wchar_t CHcySemicolonEntityValue[] = { 0x00427 };
static const wchar_t COPYEntityValue[] = { 0x000A9 };
static const wchar_t COPYSemicolonEntityValue[] = { 0x000A9 };
static const wchar_t CacuteSemicolonEntityValue[] = { 0x00106 };
static const wchar_t CapSemicolonEntityValue[] = { 0x022D2 };
static const wchar_t CapitalDifferentialDSemicolonEntityValue[] = { 0x02145 };
static const wchar_t CayleysSemicolonEntityValue[] = { 0x0212D };
static const wchar_t CcaronSemicolonEntityValue[] = { 0x0010C };
static const wchar_t CcedilEntityValue[] = { 0x000C7 };
static const wchar_t CcedilSemicolonEntityValue[] = { 0x000C7 };
static const wchar_t CcircSemicolonEntityValue[] = { 0x00108 };
static const wchar_t CconintSemicolonEntityValue[] = { 0x02230 };
static const wchar_t CdotSemicolonEntityValue[] = { 0x0010A };
static const wchar_t CedillaSemicolonEntityValue[] = { 0x000B8 };
static const wchar_t CenterDotSemicolonEntityValue[] = { 0x000B7 };
static const wchar_t CfrSemicolonEntityValue[] = { 0x0212D };
static const wchar_t ChiSemicolonEntityValue[] = { 0x003A7 };
static const wchar_t CircleDotSemicolonEntityValue[] = { 0x02299 };
static const wchar_t CircleMinusSemicolonEntityValue[] = { 0x02296 };
static const wchar_t CirclePlusSemicolonEntityValue[] = { 0x02295 };
static const wchar_t CircleTimesSemicolonEntityValue[] = { 0x02297 };
static const wchar_t ClockwiseContourIntegralSemicolonEntityValue[] = { 0x02232 };
static const wchar_t CloseCurlyDoubleQuoteSemicolonEntityValue[] = { 0x0201D };
static const wchar_t CloseCurlyQuoteSemicolonEntityValue[] = { 0x02019 };
static const wchar_t ColonSemicolonEntityValue[] = { 0x02237 };
static const wchar_t ColoneSemicolonEntityValue[] = { 0x02A74 };
static const wchar_t CongruentSemicolonEntityValue[] = { 0x02261 };
static const wchar_t ConintSemicolonEntityValue[] = { 0x0222F };
static const wchar_t ContourIntegralSemicolonEntityValue[] = { 0x0222E };
static const wchar_t CopfSemicolonEntityValue[] = { 0x02102 };
static const wchar_t CoproductSemicolonEntityValue[] = { 0x02210 };
static const wchar_t CounterClockwiseContourIntegralSemicolonEntityValue[] = { 0x02233 };
static const wchar_t CrossSemicolonEntityValue[] = { 0x02A2F };
static const wchar_t CscrSemicolonEntityValue[] = { 0x1D49E };
static const wchar_t CupSemicolonEntityValue[] = { 0x022D3 };
static const wchar_t CupCapSemicolonEntityValue[] = { 0x0224D };
static const wchar_t DDSemicolonEntityValue[] = { 0x02145 };
static const wchar_t DDotrahdSemicolonEntityValue[] = { 0x02911 };
static const wchar_t DJcySemicolonEntityValue[] = { 0x00402 };
static const wchar_t DScySemicolonEntityValue[] = { 0x00405 };
static const wchar_t DZcySemicolonEntityValue[] = { 0x0040F };
static const wchar_t DaggerSemicolonEntityValue[] = { 0x02021 };
static const wchar_t DarrSemicolonEntityValue[] = { 0x021A1 };
static const wchar_t DashvSemicolonEntityValue[] = { 0x02AE4 };
static const wchar_t DcaronSemicolonEntityValue[] = { 0x0010E };
static const wchar_t DcySemicolonEntityValue[] = { 0x00414 };
static const wchar_t DelSemicolonEntityValue[] = { 0x02207 };
static const wchar_t DeltaSemicolonEntityValue[] = { 0x00394 };
static const wchar_t DfrSemicolonEntityValue[] = { 0x1D507 };
static const wchar_t DiacriticalAcuteSemicolonEntityValue[] = { 0x000B4 };
static const wchar_t DiacriticalDotSemicolonEntityValue[] = { 0x002D9 };
static const wchar_t DiacriticalDoubleAcuteSemicolonEntityValue[] = { 0x002DD };
static const wchar_t DiacriticalGraveSemicolonEntityValue[] = { 0x00060 };
static const wchar_t DiacriticalTildeSemicolonEntityValue[] = { 0x002DC };
static const wchar_t DiamondSemicolonEntityValue[] = { 0x022C4 };
static const wchar_t DifferentialDSemicolonEntityValue[] = { 0x02146 };
static const wchar_t DopfSemicolonEntityValue[] = { 0x1D53B };
static const wchar_t DotSemicolonEntityValue[] = { 0x000A8 };
static const wchar_t DotDotSemicolonEntityValue[] = { 0x020DC };
static const wchar_t DotEqualSemicolonEntityValue[] = { 0x02250 };
static const wchar_t DoubleContourIntegralSemicolonEntityValue[] = { 0x0222F };
static const wchar_t DoubleDotSemicolonEntityValue[] = { 0x000A8 };
static const wchar_t DoubleDownArrowSemicolonEntityValue[] = { 0x021D3 };
static const wchar_t DoubleLeftArrowSemicolonEntityValue[] = { 0x021D0 };
static const wchar_t DoubleLeftRightArrowSemicolonEntityValue[] = { 0x021D4 };
static const wchar_t DoubleLeftTeeSemicolonEntityValue[] = { 0x02AE4 };
static const wchar_t DoubleLongLeftArrowSemicolonEntityValue[] = { 0x027F8 };
static const wchar_t DoubleLongLeftRightArrowSemicolonEntityValue[] = { 0x027FA };
static const wchar_t DoubleLongRightArrowSemicolonEntityValue[] = { 0x027F9 };
static const wchar_t DoubleRightArrowSemicolonEntityValue[] = { 0x021D2 };
static const wchar_t DoubleRightTeeSemicolonEntityValue[] = { 0x022A8 };
static const wchar_t DoubleUpArrowSemicolonEntityValue[] = { 0x021D1 };
static const wchar_t DoubleUpDownArrowSemicolonEntityValue[] = { 0x021D5 };
static const wchar_t DoubleVerticalBarSemicolonEntityValue[] = { 0x02225 };
static const wchar_t DownArrowSemicolonEntityValue[] = { 0x02193 };
static const wchar_t DownArrowBarSemicolonEntityValue[] = { 0x02913 };
static const wchar_t DownArrowUpArrowSemicolonEntityValue[] = { 0x021F5 };
static const wchar_t DownBreveSemicolonEntityValue[] = { 0x00311 };
static const wchar_t DownLeftRightVectorSemicolonEntityValue[] = { 0x02950 };
static const wchar_t DownLeftTeeVectorSemicolonEntityValue[] = { 0x0295E };
static const wchar_t DownLeftVectorSemicolonEntityValue[] = { 0x021BD };
static const wchar_t DownLeftVectorBarSemicolonEntityValue[] = { 0x02956 };
static const wchar_t DownRightTeeVectorSemicolonEntityValue[] = { 0x0295F };
static const wchar_t DownRightVectorSemicolonEntityValue[] = { 0x021C1 };
static const wchar_t DownRightVectorBarSemicolonEntityValue[] = { 0x02957 };
static const wchar_t DownTeeSemicolonEntityValue[] = { 0x022A4 };
static const wchar_t DownTeeArrowSemicolonEntityValue[] = { 0x021A7 };
static const wchar_t DownarrowSemicolonEntityValue[] = { 0x021D3 };
static const wchar_t DscrSemicolonEntityValue[] = { 0x1D49F };
static const wchar_t DstrokSemicolonEntityValue[] = { 0x00110 };
static const wchar_t ENGSemicolonEntityValue[] = { 0x0014A };
static const wchar_t ETHEntityValue[] = { 0x000D0 };
static const wchar_t ETHSemicolonEntityValue[] = { 0x000D0 };
static const wchar_t EacuteEntityValue[] = { 0x000C9 };
static const wchar_t EacuteSemicolonEntityValue[] = { 0x000C9 };
static const wchar_t EcaronSemicolonEntityValue[] = { 0x0011A };
static const wchar_t EcircEntityValue[] = { 0x000CA };
static const wchar_t EcircSemicolonEntityValue[] = { 0x000CA };
static const wchar_t EcySemicolonEntityValue[] = { 0x0042D };
static const wchar_t EdotSemicolonEntityValue[] = { 0x00116 };
static const wchar_t EfrSemicolonEntityValue[] = { 0x1D508 };
static const wchar_t EgraveEntityValue[] = { 0x000C8 };
static const wchar_t EgraveSemicolonEntityValue[] = { 0x000C8 };
static const wchar_t ElementSemicolonEntityValue[] = { 0x02208 };
static const wchar_t EmacrSemicolonEntityValue[] = { 0x00112 };
static const wchar_t EmptySmallSquareSemicolonEntityValue[] = { 0x025FB };
static const wchar_t EmptyVerySmallSquareSemicolonEntityValue[] = { 0x025AB };
static const wchar_t EogonSemicolonEntityValue[] = { 0x00118 };
static const wchar_t EopfSemicolonEntityValue[] = { 0x1D53C };
static const wchar_t EpsilonSemicolonEntityValue[] = { 0x00395 };
static const wchar_t EqualSemicolonEntityValue[] = { 0x02A75 };
static const wchar_t EqualTildeSemicolonEntityValue[] = { 0x02242 };
static const wchar_t EquilibriumSemicolonEntityValue[] = { 0x021CC };
static const wchar_t EscrSemicolonEntityValue[] = { 0x02130 };
static const wchar_t EsimSemicolonEntityValue[] = { 0x02A73 };
static const wchar_t EtaSemicolonEntityValue[] = { 0x00397 };
static const wchar_t EumlEntityValue[] = { 0x000CB };
static const wchar_t EumlSemicolonEntityValue[] = { 0x000CB };
static const wchar_t ExistsSemicolonEntityValue[] = { 0x02203 };
static const wchar_t ExponentialESemicolonEntityValue[] = { 0x02147 };
static const wchar_t FcySemicolonEntityValue[] = { 0x00424 };
static const wchar_t FfrSemicolonEntityValue[] = { 0x1D509 };
static const wchar_t FilledSmallSquareSemicolonEntityValue[] = { 0x025FC };
static const wchar_t FilledVerySmallSquareSemicolonEntityValue[] = { 0x025AA };
static const wchar_t FopfSemicolonEntityValue[] = { 0x1D53D };
static const wchar_t ForAllSemicolonEntityValue[] = { 0x02200 };
static const wchar_t FouriertrfSemicolonEntityValue[] = { 0x02131 };
static const wchar_t FscrSemicolonEntityValue[] = { 0x02131 };
static const wchar_t GJcySemicolonEntityValue[] = { 0x00403 };
static const wchar_t GTEntityValue[] = { 0x0003E };
static const wchar_t GTSemicolonEntityValue[] = { 0x0003E };
static const wchar_t GammaSemicolonEntityValue[] = { 0x00393 };
static const wchar_t GammadSemicolonEntityValue[] = { 0x003DC };
static const wchar_t GbreveSemicolonEntityValue[] = { 0x0011E };
static const wchar_t GcedilSemicolonEntityValue[] = { 0x00122 };
static const wchar_t GcircSemicolonEntityValue[] = { 0x0011C };
static const wchar_t GcySemicolonEntityValue[] = { 0x00413 };
static const wchar_t GdotSemicolonEntityValue[] = { 0x00120 };
static const wchar_t GfrSemicolonEntityValue[] = { 0x1D50A };
static const wchar_t GgSemicolonEntityValue[] = { 0x022D9 };
static const wchar_t GopfSemicolonEntityValue[] = { 0x1D53E };
static const wchar_t GreaterEqualSemicolonEntityValue[] = { 0x02265 };
static const wchar_t GreaterEqualLessSemicolonEntityValue[] = { 0x022DB };
static const wchar_t GreaterFullEqualSemicolonEntityValue[] = { 0x02267 };
static const wchar_t GreaterGreaterSemicolonEntityValue[] = { 0x02AA2 };
static const wchar_t GreaterLessSemicolonEntityValue[] = { 0x02277 };
static const wchar_t GreaterSlantEqualSemicolonEntityValue[] = { 0x02A7E };
static const wchar_t GreaterTildeSemicolonEntityValue[] = { 0x02273 };
static const wchar_t GscrSemicolonEntityValue[] = { 0x1D4A2 };
static const wchar_t GtSemicolonEntityValue[] = { 0x0226B };
static const wchar_t HARDcySemicolonEntityValue[] = { 0x0042A };
static const wchar_t HacekSemicolonEntityValue[] = { 0x002C7 };
static const wchar_t HatSemicolonEntityValue[] = { 0x0005E };
static const wchar_t HcircSemicolonEntityValue[] = { 0x00124 };
static const wchar_t HfrSemicolonEntityValue[] = { 0x0210C };
static const wchar_t HilbertSpaceSemicolonEntityValue[] = { 0x0210B };
static const wchar_t HopfSemicolonEntityValue[] = { 0x0210D };
static const wchar_t HorizontalLineSemicolonEntityValue[] = { 0x02500 };
static const wchar_t HscrSemicolonEntityValue[] = { 0x0210B };
static const wchar_t HstrokSemicolonEntityValue[] = { 0x00126 };
static const wchar_t HumpDownHumpSemicolonEntityValue[] = { 0x0224E };
static const wchar_t HumpEqualSemicolonEntityValue[] = { 0x0224F };
static const wchar_t IEcySemicolonEntityValue[] = { 0x00415 };
static const wchar_t IJligSemicolonEntityValue[] = { 0x00132 };
static const wchar_t IOcySemicolonEntityValue[] = { 0x00401 };
static const wchar_t IacuteEntityValue[] = { 0x000CD };
static const wchar_t IacuteSemicolonEntityValue[] = { 0x000CD };
static const wchar_t IcircEntityValue[] = { 0x000CE };
static const wchar_t IcircSemicolonEntityValue[] = { 0x000CE };
static const wchar_t IcySemicolonEntityValue[] = { 0x00418 };
static const wchar_t IdotSemicolonEntityValue[] = { 0x00130 };
static const wchar_t IfrSemicolonEntityValue[] = { 0x02111 };
static const wchar_t IgraveEntityValue[] = { 0x000CC };
static const wchar_t IgraveSemicolonEntityValue[] = { 0x000CC };
static const wchar_t ImSemicolonEntityValue[] = { 0x02111 };
static const wchar_t ImacrSemicolonEntityValue[] = { 0x0012A };
static const wchar_t ImaginaryISemicolonEntityValue[] = { 0x02148 };
static const wchar_t ImpliesSemicolonEntityValue[] = { 0x021D2 };
static const wchar_t IntSemicolonEntityValue[] = { 0x0222C };
static const wchar_t IntegralSemicolonEntityValue[] = { 0x0222B };
static const wchar_t IntersectionSemicolonEntityValue[] = { 0x022C2 };
static const wchar_t InvisibleCommaSemicolonEntityValue[] = { 0x02063 };
static const wchar_t InvisibleTimesSemicolonEntityValue[] = { 0x02062 };
static const wchar_t IogonSemicolonEntityValue[] = { 0x0012E };
static const wchar_t IopfSemicolonEntityValue[] = { 0x1D540 };
static const wchar_t IotaSemicolonEntityValue[] = { 0x00399 };
static const wchar_t IscrSemicolonEntityValue[] = { 0x02110 };
static const wchar_t ItildeSemicolonEntityValue[] = { 0x00128 };
static const wchar_t IukcySemicolonEntityValue[] = { 0x00406 };
static const wchar_t IumlEntityValue[] = { 0x000CF };
static const wchar_t IumlSemicolonEntityValue[] = { 0x000CF };
static const wchar_t JcircSemicolonEntityValue[] = { 0x00134 };
static const wchar_t JcySemicolonEntityValue[] = { 0x00419 };
static const wchar_t JfrSemicolonEntityValue[] = { 0x1D50D };
static const wchar_t JopfSemicolonEntityValue[] = { 0x1D541 };
static const wchar_t JscrSemicolonEntityValue[] = { 0x1D4A5 };
static const wchar_t JsercySemicolonEntityValue[] = { 0x00408 };
static const wchar_t JukcySemicolonEntityValue[] = { 0x00404 };
static const wchar_t KHcySemicolonEntityValue[] = { 0x00425 };
static const wchar_t KJcySemicolonEntityValue[] = { 0x0040C };
static const wchar_t KappaSemicolonEntityValue[] = { 0x0039A };
static const wchar_t KcedilSemicolonEntityValue[] = { 0x00136 };
static const wchar_t KcySemicolonEntityValue[] = { 0x0041A };
static const wchar_t KfrSemicolonEntityValue[] = { 0x1D50E };
static const wchar_t KopfSemicolonEntityValue[] = { 0x1D542 };
static const wchar_t KscrSemicolonEntityValue[] = { 0x1D4A6 };
static const wchar_t LJcySemicolonEntityValue[] = { 0x00409 };
static const wchar_t LTEntityValue[] = { 0x0003C };
static const wchar_t LTSemicolonEntityValue[] = { 0x0003C };
static const wchar_t LacuteSemicolonEntityValue[] = { 0x00139 };
static const wchar_t LambdaSemicolonEntityValue[] = { 0x0039B };
static const wchar_t LangSemicolonEntityValue[] = { 0x027EA };
static const wchar_t LaplacetrfSemicolonEntityValue[] = { 0x02112 };
static const wchar_t LarrSemicolonEntityValue[] = { 0x0219E };
static const wchar_t LcaronSemicolonEntityValue[] = { 0x0013D };
static const wchar_t LcedilSemicolonEntityValue[] = { 0x0013B };
static const wchar_t LcySemicolonEntityValue[] = { 0x0041B };
static const wchar_t LeftAngleBracketSemicolonEntityValue[] = { 0x027E8 };
static const wchar_t LeftArrowSemicolonEntityValue[] = { 0x02190 };
static const wchar_t LeftArrowBarSemicolonEntityValue[] = { 0x021E4 };
static const wchar_t LeftArrowRightArrowSemicolonEntityValue[] = { 0x021C6 };
static const wchar_t LeftCeilingSemicolonEntityValue[] = { 0x02308 };
static const wchar_t LeftDoubleBracketSemicolonEntityValue[] = { 0x027E6 };
static const wchar_t LeftDownTeeVectorSemicolonEntityValue[] = { 0x02961 };
static const wchar_t LeftDownVectorSemicolonEntityValue[] = { 0x021C3 };
static const wchar_t LeftDownVectorBarSemicolonEntityValue[] = { 0x02959 };
static const wchar_t LeftFloorSemicolonEntityValue[] = { 0x0230A };
static const wchar_t LeftRightArrowSemicolonEntityValue[] = { 0x02194 };
static const wchar_t LeftRightVectorSemicolonEntityValue[] = { 0x0294E };
static const wchar_t LeftTeeSemicolonEntityValue[] = { 0x022A3 };
static const wchar_t LeftTeeArrowSemicolonEntityValue[] = { 0x021A4 };
static const wchar_t LeftTeeVectorSemicolonEntityValue[] = { 0x0295A };
static const wchar_t LeftTriangleSemicolonEntityValue[] = { 0x022B2 };
static const wchar_t LeftTriangleBarSemicolonEntityValue[] = { 0x029CF };
static const wchar_t LeftTriangleEqualSemicolonEntityValue[] = { 0x022B4 };
static const wchar_t LeftUpDownVectorSemicolonEntityValue[] = { 0x02951 };
static const wchar_t LeftUpTeeVectorSemicolonEntityValue[] = { 0x02960 };
static const wchar_t LeftUpVectorSemicolonEntityValue[] = { 0x021BF };
static const wchar_t LeftUpVectorBarSemicolonEntityValue[] = { 0x02958 };
static const wchar_t LeftVectorSemicolonEntityValue[] = { 0x021BC };
static const wchar_t LeftVectorBarSemicolonEntityValue[] = { 0x02952 };
static const wchar_t LeftarrowSemicolonEntityValue[] = { 0x021D0 };
static const wchar_t LeftrightarrowSemicolonEntityValue[] = { 0x021D4 };
static const wchar_t LessEqualGreaterSemicolonEntityValue[] = { 0x022DA };
static const wchar_t LessFullEqualSemicolonEntityValue[] = { 0x02266 };
static const wchar_t LessGreaterSemicolonEntityValue[] = { 0x02276 };
static const wchar_t LessLessSemicolonEntityValue[] = { 0x02AA1 };
static const wchar_t LessSlantEqualSemicolonEntityValue[] = { 0x02A7D };
static const wchar_t LessTildeSemicolonEntityValue[] = { 0x02272 };
static const wchar_t LfrSemicolonEntityValue[] = { 0x1D50F };
static const wchar_t LlSemicolonEntityValue[] = { 0x022D8 };
static const wchar_t LleftarrowSemicolonEntityValue[] = { 0x021DA };
static const wchar_t LmidotSemicolonEntityValue[] = { 0x0013F };
static const wchar_t LongLeftArrowSemicolonEntityValue[] = { 0x027F5 };
static const wchar_t LongLeftRightArrowSemicolonEntityValue[] = { 0x027F7 };
static const wchar_t LongRightArrowSemicolonEntityValue[] = { 0x027F6 };
static const wchar_t LongleftarrowSemicolonEntityValue[] = { 0x027F8 };
static const wchar_t LongleftrightarrowSemicolonEntityValue[] = { 0x027FA };
static const wchar_t LongrightarrowSemicolonEntityValue[] = { 0x027F9 };
static const wchar_t LopfSemicolonEntityValue[] = { 0x1D543 };
static const wchar_t LowerLeftArrowSemicolonEntityValue[] = { 0x02199 };
static const wchar_t LowerRightArrowSemicolonEntityValue[] = { 0x02198 };
static const wchar_t LscrSemicolonEntityValue[] = { 0x02112 };
static const wchar_t LshSemicolonEntityValue[] = { 0x021B0 };
static const wchar_t LstrokSemicolonEntityValue[] = { 0x00141 };
static const wchar_t LtSemicolonEntityValue[] = { 0x0226A };
static const wchar_t MapSemicolonEntityValue[] = { 0x02905 };
static const wchar_t McySemicolonEntityValue[] = { 0x0041C };
static const wchar_t MediumSpaceSemicolonEntityValue[] = { 0x0205F };
static const wchar_t MellintrfSemicolonEntityValue[] = { 0x02133 };
static const wchar_t MfrSemicolonEntityValue[] = { 0x1D510 };
static const wchar_t MinusPlusSemicolonEntityValue[] = { 0x02213 };
static const wchar_t MopfSemicolonEntityValue[] = { 0x1D544 };
static const wchar_t MscrSemicolonEntityValue[] = { 0x02133 };
static const wchar_t MuSemicolonEntityValue[] = { 0x0039C };
static const wchar_t NJcySemicolonEntityValue[] = { 0x0040A };
static const wchar_t NacuteSemicolonEntityValue[] = { 0x00143 };
static const wchar_t NcaronSemicolonEntityValue[] = { 0x00147 };
static const wchar_t NcedilSemicolonEntityValue[] = { 0x00145 };
static const wchar_t NcySemicolonEntityValue[] = { 0x0041D };
static const wchar_t NegativeMediumSpaceSemicolonEntityValue[] = { 0x0200B };
static const wchar_t NegativeThickSpaceSemicolonEntityValue[] = { 0x0200B };
static const wchar_t NegativeThinSpaceSemicolonEntityValue[] = { 0x0200B };
static const wchar_t NegativeVeryThinSpaceSemicolonEntityValue[] = { 0x0200B };
static const wchar_t NestedGreaterGreaterSemicolonEntityValue[] = { 0x0226B };
static const wchar_t NestedLessLessSemicolonEntityValue[] = { 0x0226A };
static const wchar_t NewLineSemicolonEntityValue[] = { 0x0000A };
static const wchar_t NfrSemicolonEntityValue[] = { 0x1D511 };
static const wchar_t NoBreakSemicolonEntityValue[] = { 0x02060 };
static const wchar_t NonBreakingSpaceSemicolonEntityValue[] = { 0x000A0 };
static const wchar_t NopfSemicolonEntityValue[] = { 0x02115 };
static const wchar_t NotSemicolonEntityValue[] = { 0x02AEC };
static const wchar_t NotCongruentSemicolonEntityValue[] = { 0x02262 };
static const wchar_t NotCupCapSemicolonEntityValue[] = { 0x0226D };
static const wchar_t NotDoubleVerticalBarSemicolonEntityValue[] = { 0x02226 };
static const wchar_t NotElementSemicolonEntityValue[] = { 0x02209 };
static const wchar_t NotEqualSemicolonEntityValue[] = { 0x02260 };
static const wchar_t NotEqualTildeSemicolonEntityValue[] = { 0x02242, 0x00338 };
static const wchar_t NotExistsSemicolonEntityValue[] = { 0x02204 };
static const wchar_t NotGreaterSemicolonEntityValue[] = { 0x0226F };
static const wchar_t NotGreaterEqualSemicolonEntityValue[] = { 0x02271 };
static const wchar_t NotGreaterFullEqualSemicolonEntityValue[] = { 0x02267, 0x00338 };
static const wchar_t NotGreaterGreaterSemicolonEntityValue[] = { 0x0226B, 0x00338 };
static const wchar_t NotGreaterLessSemicolonEntityValue[] = { 0x02279 };
static const wchar_t NotGreaterSlantEqualSemicolonEntityValue[] = { 0x02A7E, 0x00338 };
static const wchar_t NotGreaterTildeSemicolonEntityValue[] = { 0x02275 };
static const wchar_t NotHumpDownHumpSemicolonEntityValue[] = { 0x0224E, 0x00338 };
static const wchar_t NotHumpEqualSemicolonEntityValue[] = { 0x0224F, 0x00338 };
static const wchar_t NotLeftTriangleSemicolonEntityValue[] = { 0x022EA };
static const wchar_t NotLeftTriangleBarSemicolonEntityValue[] = { 0x029CF, 0x00338 };
static const wchar_t NotLeftTriangleEqualSemicolonEntityValue[] = { 0x022EC };
static const wchar_t NotLessSemicolonEntityValue[] = { 0x0226E };
static const wchar_t NotLessEqualSemicolonEntityValue[] = { 0x02270 };
static const wchar_t NotLessGreaterSemicolonEntityValue[] = { 0x02278 };
static const wchar_t NotLessLessSemicolonEntityValue[] = { 0x0226A, 0x00338 };
static const wchar_t NotLessSlantEqualSemicolonEntityValue[] = { 0x02A7D, 0x00338 };
static const wchar_t NotLessTildeSemicolonEntityValue[] = { 0x02274 };
static const wchar_t NotNestedGreaterGreaterSemicolonEntityValue[] = { 0x02AA2, 0x00338 };
static const wchar_t NotNestedLessLessSemicolonEntityValue[] = { 0x02AA1, 0x00338 };
static const wchar_t NotPrecedesSemicolonEntityValue[] = { 0x02280 };
static const wchar_t NotPrecedesEqualSemicolonEntityValue[] = { 0x02AAF, 0x00338 };
static const wchar_t NotPrecedesSlantEqualSemicolonEntityValue[] = { 0x022E0 };
static const wchar_t NotReverseElementSemicolonEntityValue[] = { 0x0220C };
static const wchar_t NotRightTriangleSemicolonEntityValue[] = { 0x022EB };
static const wchar_t NotRightTriangleBarSemicolonEntityValue[] = { 0x029D0, 0x00338 };
static const wchar_t NotRightTriangleEqualSemicolonEntityValue[] = { 0x022ED };
static const wchar_t NotSquareSubsetSemicolonEntityValue[] = { 0x0228F, 0x00338 };
static const wchar_t NotSquareSubsetEqualSemicolonEntityValue[] = { 0x022E2 };
static const wchar_t NotSquareSupersetSemicolonEntityValue[] = { 0x02290, 0x00338 };
static const wchar_t NotSquareSupersetEqualSemicolonEntityValue[] = { 0x022E3 };
static const wchar_t NotSubsetSemicolonEntityValue[] = { 0x02282, 0x020D2 };
static const wchar_t NotSubsetEqualSemicolonEntityValue[] = { 0x02288 };
static const wchar_t NotSucceedsSemicolonEntityValue[] = { 0x02281 };
static const wchar_t NotSucceedsEqualSemicolonEntityValue[] = { 0x02AB0, 0x00338 };
static const wchar_t NotSucceedsSlantEqualSemicolonEntityValue[] = { 0x022E1 };
static const wchar_t NotSucceedsTildeSemicolonEntityValue[] = { 0x0227F, 0x00338 };
static const wchar_t NotSupersetSemicolonEntityValue[] = { 0x02283, 0x020D2 };
static const wchar_t NotSupersetEqualSemicolonEntityValue[] = { 0x02289 };
static const wchar_t NotTildeSemicolonEntityValue[] = { 0x02241 };
static const wchar_t NotTildeEqualSemicolonEntityValue[] = { 0x02244 };
static const wchar_t NotTildeFullEqualSemicolonEntityValue[] = { 0x02247 };
static const wchar_t NotTildeTildeSemicolonEntityValue[] = { 0x02249 };
static const wchar_t NotVerticalBarSemicolonEntityValue[] = { 0x02224 };
static const wchar_t NscrSemicolonEntityValue[] = { 0x1D4A9 };
static const wchar_t NtildeEntityValue[] = { 0x000D1 };
static const wchar_t NtildeSemicolonEntityValue[] = { 0x000D1 };
static const wchar_t NuSemicolonEntityValue[] = { 0x0039D };
static const wchar_t OEligSemicolonEntityValue[] = { 0x00152 };
static const wchar_t OacuteEntityValue[] = { 0x000D3 };
static const wchar_t OacuteSemicolonEntityValue[] = { 0x000D3 };
static const wchar_t OcircEntityValue[] = { 0x000D4 };
static const wchar_t OcircSemicolonEntityValue[] = { 0x000D4 };
static const wchar_t OcySemicolonEntityValue[] = { 0x0041E };
static const wchar_t OdblacSemicolonEntityValue[] = { 0x00150 };
static const wchar_t OfrSemicolonEntityValue[] = { 0x1D512 };
static const wchar_t OgraveEntityValue[] = { 0x000D2 };
static const wchar_t OgraveSemicolonEntityValue[] = { 0x000D2 };
static const wchar_t OmacrSemicolonEntityValue[] = { 0x0014C };
static const wchar_t OmegaSemicolonEntityValue[] = { 0x003A9 };
static const wchar_t OmicronSemicolonEntityValue[] = { 0x0039F };
static const wchar_t OopfSemicolonEntityValue[] = { 0x1D546 };
static const wchar_t OpenCurlyDoubleQuoteSemicolonEntityValue[] = { 0x0201C };
static const wchar_t OpenCurlyQuoteSemicolonEntityValue[] = { 0x02018 };
static const wchar_t OrSemicolonEntityValue[] = { 0x02A54 };
static const wchar_t OscrSemicolonEntityValue[] = { 0x1D4AA };
static const wchar_t OslashEntityValue[] = { 0x000D8 };
static const wchar_t OslashSemicolonEntityValue[] = { 0x000D8 };
static const wchar_t OtildeEntityValue[] = { 0x000D5 };
static const wchar_t OtildeSemicolonEntityValue[] = { 0x000D5 };
static const wchar_t OtimesSemicolonEntityValue[] = { 0x02A37 };
static const wchar_t OumlEntityValue[] = { 0x000D6 };
static const wchar_t OumlSemicolonEntityValue[] = { 0x000D6 };
static const wchar_t OverBarSemicolonEntityValue[] = { 0x0203E };
static const wchar_t OverBraceSemicolonEntityValue[] = { 0x023DE };
static const wchar_t OverBracketSemicolonEntityValue[] = { 0x023B4 };
static const wchar_t OverParenthesisSemicolonEntityValue[] = { 0x023DC };
static const wchar_t PartialDSemicolonEntityValue[] = { 0x02202 };
static const wchar_t PcySemicolonEntityValue[] = { 0x0041F };
static const wchar_t PfrSemicolonEntityValue[] = { 0x1D513 };
static const wchar_t PhiSemicolonEntityValue[] = { 0x003A6 };
static const wchar_t PiSemicolonEntityValue[] = { 0x003A0 };
static const wchar_t PlusMinusSemicolonEntityValue[] = { 0x000B1 };
static const wchar_t PoincareplaneSemicolonEntityValue[] = { 0x0210C };
static const wchar_t PopfSemicolonEntityValue[] = { 0x02119 };
static const wchar_t PrSemicolonEntityValue[] = { 0x02ABB };
static const wchar_t PrecedesSemicolonEntityValue[] = { 0x0227A };
static const wchar_t PrecedesEqualSemicolonEntityValue[] = { 0x02AAF };
static const wchar_t PrecedesSlantEqualSemicolonEntityValue[] = { 0x0227C };
static const wchar_t PrecedesTildeSemicolonEntityValue[] = { 0x0227E };
static const wchar_t PrimeSemicolonEntityValue[] = { 0x02033 };
static const wchar_t ProductSemicolonEntityValue[] = { 0x0220F };
static const wchar_t ProportionSemicolonEntityValue[] = { 0x02237 };
static const wchar_t ProportionalSemicolonEntityValue[] = { 0x0221D };
static const wchar_t PscrSemicolonEntityValue[] = { 0x1D4AB };
static const wchar_t PsiSemicolonEntityValue[] = { 0x003A8 };
static const wchar_t QUOTEntityValue[] = { 0x00022 };
static const wchar_t QUOTSemicolonEntityValue[] = { 0x00022 };
static const wchar_t QfrSemicolonEntityValue[] = { 0x1D514 };
static const wchar_t QopfSemicolonEntityValue[] = { 0x0211A };
static const wchar_t QscrSemicolonEntityValue[] = { 0x1D4AC };
static const wchar_t RBarrSemicolonEntityValue[] = { 0x02910 };
static const wchar_t REGEntityValue[] = { 0x000AE };
static const wchar_t REGSemicolonEntityValue[] = { 0x000AE };
static const wchar_t RacuteSemicolonEntityValue[] = { 0x00154 };
static const wchar_t RangSemicolonEntityValue[] = { 0x027EB };
static const wchar_t RarrSemicolonEntityValue[] = { 0x021A0 };
static const wchar_t RarrtlSemicolonEntityValue[] = { 0x02916 };
static const wchar_t RcaronSemicolonEntityValue[] = { 0x00158 };
static const wchar_t RcedilSemicolonEntityValue[] = { 0x00156 };
static const wchar_t RcySemicolonEntityValue[] = { 0x00420 };
static const wchar_t ReSemicolonEntityValue[] = { 0x0211C };
static const wchar_t ReverseElementSemicolonEntityValue[] = { 0x0220B };
static const wchar_t ReverseEquilibriumSemicolonEntityValue[] = { 0x021CB };
static const wchar_t ReverseUpEquilibriumSemicolonEntityValue[] = { 0x0296F };
static const wchar_t RfrSemicolonEntityValue[] = { 0x0211C };
static const wchar_t RhoSemicolonEntityValue[] = { 0x003A1 };
static const wchar_t RightAngleBracketSemicolonEntityValue[] = { 0x027E9 };
static const wchar_t RightArrowSemicolonEntityValue[] = { 0x02192 };
static const wchar_t RightArrowBarSemicolonEntityValue[] = { 0x021E5 };
static const wchar_t RightArrowLeftArrowSemicolonEntityValue[] = { 0x021C4 };
static const wchar_t RightCeilingSemicolonEntityValue[] = { 0x02309 };
static const wchar_t RightDoubleBracketSemicolonEntityValue[] = { 0x027E7 };
static const wchar_t RightDownTeeVectorSemicolonEntityValue[] = { 0x0295D };
static const wchar_t RightDownVectorSemicolonEntityValue[] = { 0x021C2 };
static const wchar_t RightDownVectorBarSemicolonEntityValue[] = { 0x02955 };
static const wchar_t RightFloorSemicolonEntityValue[] = { 0x0230B };
static const wchar_t RightTeeSemicolonEntityValue[] = { 0x022A2 };
static const wchar_t RightTeeArrowSemicolonEntityValue[] = { 0x021A6 };
static const wchar_t RightTeeVectorSemicolonEntityValue[] = { 0x0295B };
static const wchar_t RightTriangleSemicolonEntityValue[] = { 0x022B3 };
static const wchar_t RightTriangleBarSemicolonEntityValue[] = { 0x029D0 };
static const wchar_t RightTriangleEqualSemicolonEntityValue[] = { 0x022B5 };
static const wchar_t RightUpDownVectorSemicolonEntityValue[] = { 0x0294F };
static const wchar_t RightUpTeeVectorSemicolonEntityValue[] = { 0x0295C };
static const wchar_t RightUpVectorSemicolonEntityValue[] = { 0x021BE };
static const wchar_t RightUpVectorBarSemicolonEntityValue[] = { 0x02954 };
static const wchar_t RightVectorSemicolonEntityValue[] = { 0x021C0 };
static const wchar_t RightVectorBarSemicolonEntityValue[] = { 0x02953 };
static const wchar_t RightarrowSemicolonEntityValue[] = { 0x021D2 };
static const wchar_t RopfSemicolonEntityValue[] = { 0x0211D };
static const wchar_t RoundImpliesSemicolonEntityValue[] = { 0x02970 };
static const wchar_t RrightarrowSemicolonEntityValue[] = { 0x021DB };
static const wchar_t RscrSemicolonEntityValue[] = { 0x0211B };
static const wchar_t RshSemicolonEntityValue[] = { 0x021B1 };
static const wchar_t RuleDelayedSemicolonEntityValue[] = { 0x029F4 };
static const wchar_t SHCHcySemicolonEntityValue[] = { 0x00429 };
static const wchar_t SHcySemicolonEntityValue[] = { 0x00428 };
static const wchar_t SOFTcySemicolonEntityValue[] = { 0x0042C };
static const wchar_t SacuteSemicolonEntityValue[] = { 0x0015A };
static const wchar_t ScSemicolonEntityValue[] = { 0x02ABC };
static const wchar_t ScaronSemicolonEntityValue[] = { 0x00160 };
static const wchar_t ScedilSemicolonEntityValue[] = { 0x0015E };
static const wchar_t ScircSemicolonEntityValue[] = { 0x0015C };
static const wchar_t ScySemicolonEntityValue[] = { 0x00421 };
static const wchar_t SfrSemicolonEntityValue[] = { 0x1D516 };
static const wchar_t ShortDownArrowSemicolonEntityValue[] = { 0x02193 };
static const wchar_t ShortLeftArrowSemicolonEntityValue[] = { 0x02190 };
static const wchar_t ShortRightArrowSemicolonEntityValue[] = { 0x02192 };
static const wchar_t ShortUpArrowSemicolonEntityValue[] = { 0x02191 };
static const wchar_t SigmaSemicolonEntityValue[] = { 0x003A3 };
static const wchar_t SmallCircleSemicolonEntityValue[] = { 0x02218 };
static const wchar_t SopfSemicolonEntityValue[] = { 0x1D54A };
static const wchar_t SqrtSemicolonEntityValue[] = { 0x0221A };
static const wchar_t SquareSemicolonEntityValue[] = { 0x025A1 };
static const wchar_t SquareIntersectionSemicolonEntityValue[] = { 0x02293 };
static const wchar_t SquareSubsetSemicolonEntityValue[] = { 0x0228F };
static const wchar_t SquareSubsetEqualSemicolonEntityValue[] = { 0x02291 };
static const wchar_t SquareSupersetSemicolonEntityValue[] = { 0x02290 };
static const wchar_t SquareSupersetEqualSemicolonEntityValue[] = { 0x02292 };
static const wchar_t SquareUnionSemicolonEntityValue[] = { 0x02294 };
static const wchar_t SscrSemicolonEntityValue[] = { 0x1D4AE };
static const wchar_t StarSemicolonEntityValue[] = { 0x022C6 };
static const wchar_t SubSemicolonEntityValue[] = { 0x022D0 };
static const wchar_t SubsetSemicolonEntityValue[] = { 0x022D0 };
static const wchar_t SubsetEqualSemicolonEntityValue[] = { 0x02286 };
static const wchar_t SucceedsSemicolonEntityValue[] = { 0x0227B };
static const wchar_t SucceedsEqualSemicolonEntityValue[] = { 0x02AB0 };
static const wchar_t SucceedsSlantEqualSemicolonEntityValue[] = { 0x0227D };
static const wchar_t SucceedsTildeSemicolonEntityValue[] = { 0x0227F };
static const wchar_t SuchThatSemicolonEntityValue[] = { 0x0220B };
static const wchar_t SumSemicolonEntityValue[] = { 0x02211 };
static const wchar_t SupSemicolonEntityValue[] = { 0x022D1 };
static const wchar_t SupersetSemicolonEntityValue[] = { 0x02283 };
static const wchar_t SupersetEqualSemicolonEntityValue[] = { 0x02287 };
static const wchar_t SupsetSemicolonEntityValue[] = { 0x022D1 };
static const wchar_t THORNEntityValue[] = { 0x000DE };
static const wchar_t THORNSemicolonEntityValue[] = { 0x000DE };
static const wchar_t TRADESemicolonEntityValue[] = { 0x02122 };
static const wchar_t TSHcySemicolonEntityValue[] = { 0x0040B };
static const wchar_t TScySemicolonEntityValue[] = { 0x00426 };
static const wchar_t TabSemicolonEntityValue[] = { 0x00009 };
static const wchar_t TauSemicolonEntityValue[] = { 0x003A4 };
static const wchar_t TcaronSemicolonEntityValue[] = { 0x00164 };
static const wchar_t TcedilSemicolonEntityValue[] = { 0x00162 };
static const wchar_t TcySemicolonEntityValue[] = { 0x00422 };
static const wchar_t TfrSemicolonEntityValue[] = { 0x1D517 };
static const wchar_t ThereforeSemicolonEntityValue[] = { 0x02234 };
static const wchar_t ThetaSemicolonEntityValue[] = { 0x00398 };
static const wchar_t ThickSpaceSemicolonEntityValue[] = { 0x0205F, 0x0200A };
static const wchar_t ThinSpaceSemicolonEntityValue[] = { 0x02009 };
static const wchar_t TildeSemicolonEntityValue[] = { 0x0223C };
static const wchar_t TildeEqualSemicolonEntityValue[] = { 0x02243 };
static const wchar_t TildeFullEqualSemicolonEntityValue[] = { 0x02245 };
static const wchar_t TildeTildeSemicolonEntityValue[] = { 0x02248 };
static const wchar_t TopfSemicolonEntityValue[] = { 0x1D54B };
static const wchar_t TripleDotSemicolonEntityValue[] = { 0x020DB };
static const wchar_t TscrSemicolonEntityValue[] = { 0x1D4AF };
static const wchar_t TstrokSemicolonEntityValue[] = { 0x00166 };
static const wchar_t UacuteEntityValue[] = { 0x000DA };
static const wchar_t UacuteSemicolonEntityValue[] = { 0x000DA };
static const wchar_t UarrSemicolonEntityValue[] = { 0x0219F };
static const wchar_t UarrocirSemicolonEntityValue[] = { 0x02949 };
static const wchar_t UbrcySemicolonEntityValue[] = { 0x0040E };
static const wchar_t UbreveSemicolonEntityValue[] = { 0x0016C };
static const wchar_t UcircEntityValue[] = { 0x000DB };
static const wchar_t UcircSemicolonEntityValue[] = { 0x000DB };
static const wchar_t UcySemicolonEntityValue[] = { 0x00423 };
static const wchar_t UdblacSemicolonEntityValue[] = { 0x00170 };
static const wchar_t UfrSemicolonEntityValue[] = { 0x1D518 };
static const wchar_t UgraveEntityValue[] = { 0x000D9 };
static const wchar_t UgraveSemicolonEntityValue[] = { 0x000D9 };
static const wchar_t UmacrSemicolonEntityValue[] = { 0x0016A };
static const wchar_t UnderBarSemicolonEntityValue[] = { 0x0005F };
static const wchar_t UnderBraceSemicolonEntityValue[] = { 0x023DF };
static const wchar_t UnderBracketSemicolonEntityValue[] = { 0x023B5 };
static const wchar_t UnderParenthesisSemicolonEntityValue[] = { 0x023DD };
static const wchar_t UnionSemicolonEntityValue[] = { 0x022C3 };
static const wchar_t UnionPlusSemicolonEntityValue[] = { 0x0228E };
static const wchar_t UogonSemicolonEntityValue[] = { 0x00172 };
static const wchar_t UopfSemicolonEntityValue[] = { 0x1D54C };
static const wchar_t UpArrowSemicolonEntityValue[] = { 0x02191 };
static const wchar_t UpArrowBarSemicolonEntityValue[] = { 0x02912 };
static const wchar_t UpArrowDownArrowSemicolonEntityValue[] = { 0x021C5 };
static const wchar_t UpDownArrowSemicolonEntityValue[] = { 0x02195 };
static const wchar_t UpEquilibriumSemicolonEntityValue[] = { 0x0296E };
static const wchar_t UpTeeSemicolonEntityValue[] = { 0x022A5 };
static const wchar_t UpTeeArrowSemicolonEntityValue[] = { 0x021A5 };
static const wchar_t UparrowSemicolonEntityValue[] = { 0x021D1 };
static const wchar_t UpdownarrowSemicolonEntityValue[] = { 0x021D5 };
static const wchar_t UpperLeftArrowSemicolonEntityValue[] = { 0x02196 };
static const wchar_t UpperRightArrowSemicolonEntityValue[] = { 0x02197 };
static const wchar_t UpsiSemicolonEntityValue[] = { 0x003D2 };
static const wchar_t UpsilonSemicolonEntityValue[] = { 0x003A5 };
static const wchar_t UringSemicolonEntityValue[] = { 0x0016E };
static const wchar_t UscrSemicolonEntityValue[] = { 0x1D4B0 };
static const wchar_t UtildeSemicolonEntityValue[] = { 0x00168 };
static const wchar_t UumlEntityValue[] = { 0x000DC };
static const wchar_t UumlSemicolonEntityValue[] = { 0x000DC };
static const wchar_t VDashSemicolonEntityValue[] = { 0x022AB };
static const wchar_t VbarSemicolonEntityValue[] = { 0x02AEB };
static const wchar_t VcySemicolonEntityValue[] = { 0x00412 };
static const wchar_t VdashSemicolonEntityValue[] = { 0x022A9 };
static const wchar_t VdashlSemicolonEntityValue[] = { 0x02AE6 };
static const wchar_t VeeSemicolonEntityValue[] = { 0x022C1 };
static const wchar_t VerbarSemicolonEntityValue[] = { 0x02016 };
static const wchar_t VertSemicolonEntityValue[] = { 0x02016 };
static const wchar_t VerticalBarSemicolonEntityValue[] = { 0x02223 };
static const wchar_t VerticalLineSemicolonEntityValue[] = { 0x0007C };
static const wchar_t VerticalSeparatorSemicolonEntityValue[] = { 0x02758 };
static const wchar_t VerticalTildeSemicolonEntityValue[] = { 0x02240 };
static const wchar_t VeryThinSpaceSemicolonEntityValue[] = { 0x0200A };
static const wchar_t VfrSemicolonEntityValue[] = { 0x1D519 };
static const wchar_t VopfSemicolonEntityValue[] = { 0x1D54D };
static const wchar_t VscrSemicolonEntityValue[] = { 0x1D4B1 };
static const wchar_t VvdashSemicolonEntityValue[] = { 0x022AA };
static const wchar_t WcircSemicolonEntityValue[] = { 0x00174 };
static const wchar_t WedgeSemicolonEntityValue[] = { 0x022C0 };
static const wchar_t WfrSemicolonEntityValue[] = { 0x1D51A };
static const wchar_t WopfSemicolonEntityValue[] = { 0x1D54E };
static const wchar_t WscrSemicolonEntityValue[] = { 0x1D4B2 };
static const wchar_t XfrSemicolonEntityValue[] = { 0x1D51B };
static const wchar_t XiSemicolonEntityValue[] = { 0x0039E };
static const wchar_t XopfSemicolonEntityValue[] = { 0x1D54F };
static const wchar_t XscrSemicolonEntityValue[] = { 0x1D4B3 };
static const wchar_t YAcySemicolonEntityValue[] = { 0x0042F };
static const wchar_t YIcySemicolonEntityValue[] = { 0x00407 };
static const wchar_t YUcySemicolonEntityValue[] = { 0x0042E };
static const wchar_t YacuteEntityValue[] = { 0x000DD };
static const wchar_t YacuteSemicolonEntityValue[] = { 0x000DD };
static const wchar_t YcircSemicolonEntityValue[] = { 0x00176 };
static const wchar_t YcySemicolonEntityValue[] = { 0x0042B };
static const wchar_t YfrSemicolonEntityValue[] = { 0x1D51C };
static const wchar_t YopfSemicolonEntityValue[] = { 0x1D550 };
static const wchar_t YscrSemicolonEntityValue[] = { 0x1D4B4 };
static const wchar_t YumlSemicolonEntityValue[] = { 0x00178 };
static const wchar_t ZHcySemicolonEntityValue[] = { 0x00416 };
static const wchar_t ZacuteSemicolonEntityValue[] = { 0x00179 };
static const wchar_t ZcaronSemicolonEntityValue[] = { 0x0017D };
static const wchar_t ZcySemicolonEntityValue[] = { 0x00417 };
static const wchar_t ZdotSemicolonEntityValue[] = { 0x0017B };
static const wchar_t ZeroWidthSpaceSemicolonEntityValue[] = { 0x0200B };
static const wchar_t ZetaSemicolonEntityValue[] = { 0x00396 };
static const wchar_t ZfrSemicolonEntityValue[] = { 0x02128 };
static const wchar_t ZopfSemicolonEntityValue[] = { 0x02124 };
static const wchar_t ZscrSemicolonEntityValue[] = { 0x1D4B5 };
static const wchar_t aacuteEntityValue[] = { 0x000E1 };
static const wchar_t aacuteSemicolonEntityValue[] = { 0x000E1 };
static const wchar_t abreveSemicolonEntityValue[] = { 0x00103 };
static const wchar_t acSemicolonEntityValue[] = { 0x0223E };
static const wchar_t acESemicolonEntityValue[] = { 0x0223E, 0x00333 };
static const wchar_t acdSemicolonEntityValue[] = { 0x0223F };
static const wchar_t acircEntityValue[] = { 0x000E2 };
static const wchar_t acircSemicolonEntityValue[] = { 0x000E2 };
static const wchar_t acuteEntityValue[] = { 0x000B4 };
static const wchar_t acuteSemicolonEntityValue[] = { 0x000B4 };
static const wchar_t acySemicolonEntityValue[] = { 0x00430 };
static const wchar_t aeligEntityValue[] = { 0x000E6 };
static const wchar_t aeligSemicolonEntityValue[] = { 0x000E6 };
static const wchar_t afSemicolonEntityValue[] = { 0x02061 };
static const wchar_t afrSemicolonEntityValue[] = { 0x1D51E };
static const wchar_t agraveEntityValue[] = { 0x000E0 };
static const wchar_t agraveSemicolonEntityValue[] = { 0x000E0 };
static const wchar_t alefsymSemicolonEntityValue[] = { 0x02135 };
static const wchar_t alephSemicolonEntityValue[] = { 0x02135 };
static const wchar_t alphaSemicolonEntityValue[] = { 0x003B1 };
static const wchar_t amacrSemicolonEntityValue[] = { 0x00101 };
static const wchar_t amalgSemicolonEntityValue[] = { 0x02A3F };
static const wchar_t ampEntityValue[] = { 0x00026 };
static const wchar_t ampSemicolonEntityValue[] = { 0x00026 };
static const wchar_t andSemicolonEntityValue[] = { 0x02227 };
static const wchar_t andandSemicolonEntityValue[] = { 0x02A55 };
static const wchar_t anddSemicolonEntityValue[] = { 0x02A5C };
static const wchar_t andslopeSemicolonEntityValue[] = { 0x02A58 };
static const wchar_t andvSemicolonEntityValue[] = { 0x02A5A };
static const wchar_t angSemicolonEntityValue[] = { 0x02220 };
static const wchar_t angeSemicolonEntityValue[] = { 0x029A4 };
static const wchar_t angleSemicolonEntityValue[] = { 0x02220 };
static const wchar_t angmsdSemicolonEntityValue[] = { 0x02221 };
static const wchar_t angmsdaaSemicolonEntityValue[] = { 0x029A8 };
static const wchar_t angmsdabSemicolonEntityValue[] = { 0x029A9 };
static const wchar_t angmsdacSemicolonEntityValue[] = { 0x029AA };
static const wchar_t angmsdadSemicolonEntityValue[] = { 0x029AB };
static const wchar_t angmsdaeSemicolonEntityValue[] = { 0x029AC };
static const wchar_t angmsdafSemicolonEntityValue[] = { 0x029AD };
static const wchar_t angmsdagSemicolonEntityValue[] = { 0x029AE };
static const wchar_t angmsdahSemicolonEntityValue[] = { 0x029AF };
static const wchar_t angrtSemicolonEntityValue[] = { 0x0221F };
static const wchar_t angrtvbSemicolonEntityValue[] = { 0x022BE };
static const wchar_t angrtvbdSemicolonEntityValue[] = { 0x0299D };
static const wchar_t angsphSemicolonEntityValue[] = { 0x02222 };
static const wchar_t angstSemicolonEntityValue[] = { 0x000C5 };
static const wchar_t angzarrSemicolonEntityValue[] = { 0x0237C };
static const wchar_t aogonSemicolonEntityValue[] = { 0x00105 };
static const wchar_t aopfSemicolonEntityValue[] = { 0x1D552 };
static const wchar_t apSemicolonEntityValue[] = { 0x02248 };
static const wchar_t apESemicolonEntityValue[] = { 0x02A70 };
static const wchar_t apacirSemicolonEntityValue[] = { 0x02A6F };
static const wchar_t apeSemicolonEntityValue[] = { 0x0224A };
static const wchar_t apidSemicolonEntityValue[] = { 0x0224B };
static const wchar_t aposSemicolonEntityValue[] = { 0x00027 };
static const wchar_t approxSemicolonEntityValue[] = { 0x02248 };
static const wchar_t approxeqSemicolonEntityValue[] = { 0x0224A };
static const wchar_t aringEntityValue[] = { 0x000E5 };
static const wchar_t aringSemicolonEntityValue[] = { 0x000E5 };
static const wchar_t ascrSemicolonEntityValue[] = { 0x1D4B6 };
static const wchar_t astSemicolonEntityValue[] = { 0x0002A };
static const wchar_t asympSemicolonEntityValue[] = { 0x02248 };
static const wchar_t asympeqSemicolonEntityValue[] = { 0x0224D };
static const wchar_t atildeEntityValue[] = { 0x000E3 };
static const wchar_t atildeSemicolonEntityValue[] = { 0x000E3 };
static const wchar_t aumlEntityValue[] = { 0x000E4 };
static const wchar_t aumlSemicolonEntityValue[] = { 0x000E4 };
static const wchar_t awconintSemicolonEntityValue[] = { 0x02233 };
static const wchar_t awintSemicolonEntityValue[] = { 0x02A11 };
static const wchar_t bNotSemicolonEntityValue[] = { 0x02AED };
static const wchar_t backcongSemicolonEntityValue[] = { 0x0224C };
static const wchar_t backepsilonSemicolonEntityValue[] = { 0x003F6 };
static const wchar_t backprimeSemicolonEntityValue[] = { 0x02035 };
static const wchar_t backsimSemicolonEntityValue[] = { 0x0223D };
static const wchar_t backsimeqSemicolonEntityValue[] = { 0x022CD };
static const wchar_t barveeSemicolonEntityValue[] = { 0x022BD };
static const wchar_t barwedSemicolonEntityValue[] = { 0x02305 };
static const wchar_t barwedgeSemicolonEntityValue[] = { 0x02305 };
static const wchar_t bbrkSemicolonEntityValue[] = { 0x023B5 };
static const wchar_t bbrktbrkSemicolonEntityValue[] = { 0x023B6 };
static const wchar_t bcongSemicolonEntityValue[] = { 0x0224C };
static const wchar_t bcySemicolonEntityValue[] = { 0x00431 };
static const wchar_t bdquoSemicolonEntityValue[] = { 0x0201E };
static const wchar_t becausSemicolonEntityValue[] = { 0x02235 };
static const wchar_t becauseSemicolonEntityValue[] = { 0x02235 };
static const wchar_t bemptyvSemicolonEntityValue[] = { 0x029B0 };
static const wchar_t bepsiSemicolonEntityValue[] = { 0x003F6 };
static const wchar_t bernouSemicolonEntityValue[] = { 0x0212C };
static const wchar_t betaSemicolonEntityValue[] = { 0x003B2 };
static const wchar_t bethSemicolonEntityValue[] = { 0x02136 };
static const wchar_t betweenSemicolonEntityValue[] = { 0x0226C };
static const wchar_t bfrSemicolonEntityValue[] = { 0x1D51F };
static const wchar_t bigcapSemicolonEntityValue[] = { 0x022C2 };
static const wchar_t bigcircSemicolonEntityValue[] = { 0x025EF };
static const wchar_t bigcupSemicolonEntityValue[] = { 0x022C3 };
static const wchar_t bigodotSemicolonEntityValue[] = { 0x02A00 };
static const wchar_t bigoplusSemicolonEntityValue[] = { 0x02A01 };
static const wchar_t bigotimesSemicolonEntityValue[] = { 0x02A02 };
static const wchar_t bigsqcupSemicolonEntityValue[] = { 0x02A06 };
static const wchar_t bigstarSemicolonEntityValue[] = { 0x02605 };
static const wchar_t bigtriangledownSemicolonEntityValue[] = { 0x025BD };
static const wchar_t bigtriangleupSemicolonEntityValue[] = { 0x025B3 };
static const wchar_t biguplusSemicolonEntityValue[] = { 0x02A04 };
static const wchar_t bigveeSemicolonEntityValue[] = { 0x022C1 };
static const wchar_t bigwedgeSemicolonEntityValue[] = { 0x022C0 };
static const wchar_t bkarowSemicolonEntityValue[] = { 0x0290D };
static const wchar_t blacklozengeSemicolonEntityValue[] = { 0x029EB };
static const wchar_t blacksquareSemicolonEntityValue[] = { 0x025AA };
static const wchar_t blacktriangleSemicolonEntityValue[] = { 0x025B4 };
static const wchar_t blacktriangledownSemicolonEntityValue[] = { 0x025BE };
static const wchar_t blacktriangleleftSemicolonEntityValue[] = { 0x025C2 };
static const wchar_t blacktrianglerightSemicolonEntityValue[] = { 0x025B8 };
static const wchar_t blankSemicolonEntityValue[] = { 0x02423 };
static const wchar_t blk12SemicolonEntityValue[] = { 0x02592 };
static const wchar_t blk14SemicolonEntityValue[] = { 0x02591 };
static const wchar_t blk34SemicolonEntityValue[] = { 0x02593 };
static const wchar_t blockSemicolonEntityValue[] = { 0x02588 };
static const wchar_t bneSemicolonEntityValue[] = { 0x0003D, 0x020E5 };
static const wchar_t bnequivSemicolonEntityValue[] = { 0x02261, 0x020E5 };
static const wchar_t bnotSemicolonEntityValue[] = { 0x02310 };
static const wchar_t bopfSemicolonEntityValue[] = { 0x1D553 };
static const wchar_t botSemicolonEntityValue[] = { 0x022A5 };
static const wchar_t bottomSemicolonEntityValue[] = { 0x022A5 };
static const wchar_t bowtieSemicolonEntityValue[] = { 0x022C8 };
static const wchar_t boxDLSemicolonEntityValue[] = { 0x02557 };
static const wchar_t boxDRSemicolonEntityValue[] = { 0x02554 };
static const wchar_t boxDlSemicolonEntityValue[] = { 0x02556 };
static const wchar_t boxDrSemicolonEntityValue[] = { 0x02553 };
static const wchar_t boxHSemicolonEntityValue[] = { 0x02550 };
static const wchar_t boxHDSemicolonEntityValue[] = { 0x02566 };
static const wchar_t boxHUSemicolonEntityValue[] = { 0x02569 };
static const wchar_t boxHdSemicolonEntityValue[] = { 0x02564 };
static const wchar_t boxHuSemicolonEntityValue[] = { 0x02567 };
static const wchar_t boxULSemicolonEntityValue[] = { 0x0255D };
static const wchar_t boxURSemicolonEntityValue[] = { 0x0255A };
static const wchar_t boxUlSemicolonEntityValue[] = { 0x0255C };
static const wchar_t boxUrSemicolonEntityValue[] = { 0x02559 };
static const wchar_t boxVSemicolonEntityValue[] = { 0x02551 };
static const wchar_t boxVHSemicolonEntityValue[] = { 0x0256C };
static const wchar_t boxVLSemicolonEntityValue[] = { 0x02563 };
static const wchar_t boxVRSemicolonEntityValue[] = { 0x02560 };
static const wchar_t boxVhSemicolonEntityValue[] = { 0x0256B };
static const wchar_t boxVlSemicolonEntityValue[] = { 0x02562 };
static const wchar_t boxVrSemicolonEntityValue[] = { 0x0255F };
static const wchar_t boxboxSemicolonEntityValue[] = { 0x029C9 };
static const wchar_t boxdLSemicolonEntityValue[] = { 0x02555 };
static const wchar_t boxdRSemicolonEntityValue[] = { 0x02552 };
static const wchar_t boxdlSemicolonEntityValue[] = { 0x02510 };
static const wchar_t boxdrSemicolonEntityValue[] = { 0x0250C };
static const wchar_t boxhSemicolonEntityValue[] = { 0x02500 };
static const wchar_t boxhDSemicolonEntityValue[] = { 0x02565 };
static const wchar_t boxhUSemicolonEntityValue[] = { 0x02568 };
static const wchar_t boxhdSemicolonEntityValue[] = { 0x0252C };
static const wchar_t boxhuSemicolonEntityValue[] = { 0x02534 };
static const wchar_t boxminusSemicolonEntityValue[] = { 0x0229F };
static const wchar_t boxplusSemicolonEntityValue[] = { 0x0229E };
static const wchar_t boxtimesSemicolonEntityValue[] = { 0x022A0 };
static const wchar_t boxuLSemicolonEntityValue[] = { 0x0255B };
static const wchar_t boxuRSemicolonEntityValue[] = { 0x02558 };
static const wchar_t boxulSemicolonEntityValue[] = { 0x02518 };
static const wchar_t boxurSemicolonEntityValue[] = { 0x02514 };
static const wchar_t boxvSemicolonEntityValue[] = { 0x02502 };
static const wchar_t boxvHSemicolonEntityValue[] = { 0x0256A };
static const wchar_t boxvLSemicolonEntityValue[] = { 0x02561 };
static const wchar_t boxvRSemicolonEntityValue[] = { 0x0255E };
static const wchar_t boxvhSemicolonEntityValue[] = { 0x0253C };
static const wchar_t boxvlSemicolonEntityValue[] = { 0x02524 };
static const wchar_t boxvrSemicolonEntityValue[] = { 0x0251C };
static const wchar_t bprimeSemicolonEntityValue[] = { 0x02035 };
static const wchar_t breveSemicolonEntityValue[] = { 0x002D8 };
static const wchar_t brvbarEntityValue[] = { 0x000A6 };
static const wchar_t brvbarSemicolonEntityValue[] = { 0x000A6 };
static const wchar_t bscrSemicolonEntityValue[] = { 0x1D4B7 };
static const wchar_t bsemiSemicolonEntityValue[] = { 0x0204F };
static const wchar_t bsimSemicolonEntityValue[] = { 0x0223D };
static const wchar_t bsimeSemicolonEntityValue[] = { 0x022CD };
static const wchar_t bsolSemicolonEntityValue[] = { 0x0005C };
static const wchar_t bsolbSemicolonEntityValue[] = { 0x029C5 };
static const wchar_t bsolhsubSemicolonEntityValue[] = { 0x027C8 };
static const wchar_t bullSemicolonEntityValue[] = { 0x02022 };
static const wchar_t bulletSemicolonEntityValue[] = { 0x02022 };
static const wchar_t bumpSemicolonEntityValue[] = { 0x0224E };
static const wchar_t bumpESemicolonEntityValue[] = { 0x02AAE };
static const wchar_t bumpeSemicolonEntityValue[] = { 0x0224F };
static const wchar_t bumpeqSemicolonEntityValue[] = { 0x0224F };
static const wchar_t cacuteSemicolonEntityValue[] = { 0x00107 };
static const wchar_t capSemicolonEntityValue[] = { 0x02229 };
static const wchar_t capandSemicolonEntityValue[] = { 0x02A44 };
static const wchar_t capbrcupSemicolonEntityValue[] = { 0x02A49 };
static const wchar_t capcapSemicolonEntityValue[] = { 0x02A4B };
static const wchar_t capcupSemicolonEntityValue[] = { 0x02A47 };
static const wchar_t capdotSemicolonEntityValue[] = { 0x02A40 };
static const wchar_t capsSemicolonEntityValue[] = { 0x02229, 0x0FE00 };
static const wchar_t caretSemicolonEntityValue[] = { 0x02041 };
static const wchar_t caronSemicolonEntityValue[] = { 0x002C7 };
static const wchar_t ccapsSemicolonEntityValue[] = { 0x02A4D };
static const wchar_t ccaronSemicolonEntityValue[] = { 0x0010D };
static const wchar_t ccedilEntityValue[] = { 0x000E7 };
static const wchar_t ccedilSemicolonEntityValue[] = { 0x000E7 };
static const wchar_t ccircSemicolonEntityValue[] = { 0x00109 };
static const wchar_t ccupsSemicolonEntityValue[] = { 0x02A4C };
static const wchar_t ccupssmSemicolonEntityValue[] = { 0x02A50 };
static const wchar_t cdotSemicolonEntityValue[] = { 0x0010B };
static const wchar_t cedilEntityValue[] = { 0x000B8 };
static const wchar_t cedilSemicolonEntityValue[] = { 0x000B8 };
static const wchar_t cemptyvSemicolonEntityValue[] = { 0x029B2 };
static const wchar_t centEntityValue[] = { 0x000A2 };
static const wchar_t centSemicolonEntityValue[] = { 0x000A2 };
static const wchar_t centerdotSemicolonEntityValue[] = { 0x000B7 };
static const wchar_t cfrSemicolonEntityValue[] = { 0x1D520 };
static const wchar_t chcySemicolonEntityValue[] = { 0x00447 };
static const wchar_t checkSemicolonEntityValue[] = { 0x02713 };
static const wchar_t checkmarkSemicolonEntityValue[] = { 0x02713 };
static const wchar_t chiSemicolonEntityValue[] = { 0x003C7 };
static const wchar_t cirSemicolonEntityValue[] = { 0x025CB };
static const wchar_t cirESemicolonEntityValue[] = { 0x029C3 };
static const wchar_t circSemicolonEntityValue[] = { 0x002C6 };
static const wchar_t circeqSemicolonEntityValue[] = { 0x02257 };
static const wchar_t circlearrowleftSemicolonEntityValue[] = { 0x021BA };
static const wchar_t circlearrowrightSemicolonEntityValue[] = { 0x021BB };
static const wchar_t circledRSemicolonEntityValue[] = { 0x000AE };
static const wchar_t circledSSemicolonEntityValue[] = { 0x024C8 };
static const wchar_t circledastSemicolonEntityValue[] = { 0x0229B };
static const wchar_t circledcircSemicolonEntityValue[] = { 0x0229A };
static const wchar_t circleddashSemicolonEntityValue[] = { 0x0229D };
static const wchar_t cireSemicolonEntityValue[] = { 0x02257 };
static const wchar_t cirfnintSemicolonEntityValue[] = { 0x02A10 };
static const wchar_t cirmidSemicolonEntityValue[] = { 0x02AEF };
static const wchar_t cirscirSemicolonEntityValue[] = { 0x029C2 };
static const wchar_t clubsSemicolonEntityValue[] = { 0x02663 };
static const wchar_t clubsuitSemicolonEntityValue[] = { 0x02663 };
static const wchar_t colonSemicolonEntityValue[] = { 0x0003A };
static const wchar_t coloneSemicolonEntityValue[] = { 0x02254 };
static const wchar_t coloneqSemicolonEntityValue[] = { 0x02254 };
static const wchar_t commaSemicolonEntityValue[] = { 0x0002C };
static const wchar_t commatSemicolonEntityValue[] = { 0x00040 };
static const wchar_t compSemicolonEntityValue[] = { 0x02201 };
static const wchar_t compfnSemicolonEntityValue[] = { 0x02218 };
static const wchar_t complementSemicolonEntityValue[] = { 0x02201 };
static const wchar_t complexesSemicolonEntityValue[] = { 0x02102 };
static const wchar_t congSemicolonEntityValue[] = { 0x02245 };
static const wchar_t congdotSemicolonEntityValue[] = { 0x02A6D };
static const wchar_t conintSemicolonEntityValue[] = { 0x0222E };
static const wchar_t copfSemicolonEntityValue[] = { 0x1D554 };
static const wchar_t coprodSemicolonEntityValue[] = { 0x02210 };
static const wchar_t copyEntityValue[] = { 0x000A9 };
static const wchar_t copySemicolonEntityValue[] = { 0x000A9 };
static const wchar_t copysrSemicolonEntityValue[] = { 0x02117 };
static const wchar_t crarrSemicolonEntityValue[] = { 0x021B5 };
static const wchar_t crossSemicolonEntityValue[] = { 0x02717 };
static const wchar_t cscrSemicolonEntityValue[] = { 0x1D4B8 };
static const wchar_t csubSemicolonEntityValue[] = { 0x02ACF };
static const wchar_t csubeSemicolonEntityValue[] = { 0x02AD1 };
static const wchar_t csupSemicolonEntityValue[] = { 0x02AD0 };
static const wchar_t csupeSemicolonEntityValue[] = { 0x02AD2 };
static const wchar_t ctdotSemicolonEntityValue[] = { 0x022EF };
static const wchar_t cudarrlSemicolonEntityValue[] = { 0x02938 };
static const wchar_t cudarrrSemicolonEntityValue[] = { 0x02935 };
static const wchar_t cueprSemicolonEntityValue[] = { 0x022DE };
static const wchar_t cuescSemicolonEntityValue[] = { 0x022DF };
static const wchar_t cularrSemicolonEntityValue[] = { 0x021B6 };
static const wchar_t cularrpSemicolonEntityValue[] = { 0x0293D };
static const wchar_t cupSemicolonEntityValue[] = { 0x0222A };
static const wchar_t cupbrcapSemicolonEntityValue[] = { 0x02A48 };
static const wchar_t cupcapSemicolonEntityValue[] = { 0x02A46 };
static const wchar_t cupcupSemicolonEntityValue[] = { 0x02A4A };
static const wchar_t cupdotSemicolonEntityValue[] = { 0x0228D };
static const wchar_t cuporSemicolonEntityValue[] = { 0x02A45 };
static const wchar_t cupsSemicolonEntityValue[] = { 0x0222A, 0x0FE00 };
static const wchar_t curarrSemicolonEntityValue[] = { 0x021B7 };
static const wchar_t curarrmSemicolonEntityValue[] = { 0x0293C };
static const wchar_t curlyeqprecSemicolonEntityValue[] = { 0x022DE };
static const wchar_t curlyeqsuccSemicolonEntityValue[] = { 0x022DF };
static const wchar_t curlyveeSemicolonEntityValue[] = { 0x022CE };
static const wchar_t curlywedgeSemicolonEntityValue[] = { 0x022CF };
static const wchar_t currenEntityValue[] = { 0x000A4 };
static const wchar_t currenSemicolonEntityValue[] = { 0x000A4 };
static const wchar_t curvearrowleftSemicolonEntityValue[] = { 0x021B6 };
static const wchar_t curvearrowrightSemicolonEntityValue[] = { 0x021B7 };
static const wchar_t cuveeSemicolonEntityValue[] = { 0x022CE };
static const wchar_t cuwedSemicolonEntityValue[] = { 0x022CF };
static const wchar_t cwconintSemicolonEntityValue[] = { 0x02232 };
static const wchar_t cwintSemicolonEntityValue[] = { 0x02231 };
static const wchar_t cylctySemicolonEntityValue[] = { 0x0232D };
static const wchar_t dArrSemicolonEntityValue[] = { 0x021D3 };
static const wchar_t dHarSemicolonEntityValue[] = { 0x02965 };
static const wchar_t daggerSemicolonEntityValue[] = { 0x02020 };
static const wchar_t dalethSemicolonEntityValue[] = { 0x02138 };
static const wchar_t darrSemicolonEntityValue[] = { 0x02193 };
static const wchar_t dashSemicolonEntityValue[] = { 0x02010 };
static const wchar_t dashvSemicolonEntityValue[] = { 0x022A3 };
static const wchar_t dbkarowSemicolonEntityValue[] = { 0x0290F };
static const wchar_t dblacSemicolonEntityValue[] = { 0x002DD };
static const wchar_t dcaronSemicolonEntityValue[] = { 0x0010F };
static const wchar_t dcySemicolonEntityValue[] = { 0x00434 };
static const wchar_t ddSemicolonEntityValue[] = { 0x02146 };
static const wchar_t ddaggerSemicolonEntityValue[] = { 0x02021 };
static const wchar_t ddarrSemicolonEntityValue[] = { 0x021CA };
static const wchar_t ddotseqSemicolonEntityValue[] = { 0x02A77 };
static const wchar_t degEntityValue[] = { 0x000B0 };
static const wchar_t degSemicolonEntityValue[] = { 0x000B0 };
static const wchar_t deltaSemicolonEntityValue[] = { 0x003B4 };
static const wchar_t demptyvSemicolonEntityValue[] = { 0x029B1 };
static const wchar_t dfishtSemicolonEntityValue[] = { 0x0297F };
static const wchar_t dfrSemicolonEntityValue[] = { 0x1D521 };
static const wchar_t dharlSemicolonEntityValue[] = { 0x021C3 };
static const wchar_t dharrSemicolonEntityValue[] = { 0x021C2 };
static const wchar_t diamSemicolonEntityValue[] = { 0x022C4 };
static const wchar_t diamondSemicolonEntityValue[] = { 0x022C4 };
static const wchar_t diamondsuitSemicolonEntityValue[] = { 0x02666 };
static const wchar_t diamsSemicolonEntityValue[] = { 0x02666 };
static const wchar_t dieSemicolonEntityValue[] = { 0x000A8 };
static const wchar_t digammaSemicolonEntityValue[] = { 0x003DD };
static const wchar_t disinSemicolonEntityValue[] = { 0x022F2 };
static const wchar_t divSemicolonEntityValue[] = { 0x000F7 };
static const wchar_t divideEntityValue[] = { 0x000F7 };
static const wchar_t divideSemicolonEntityValue[] = { 0x000F7 };
static const wchar_t divideontimesSemicolonEntityValue[] = { 0x022C7 };
static const wchar_t divonxSemicolonEntityValue[] = { 0x022C7 };
static const wchar_t djcySemicolonEntityValue[] = { 0x00452 };
static const wchar_t dlcornSemicolonEntityValue[] = { 0x0231E };
static const wchar_t dlcropSemicolonEntityValue[] = { 0x0230D };
static const wchar_t dollarSemicolonEntityValue[] = { 0x00024 };
static const wchar_t dopfSemicolonEntityValue[] = { 0x1D555 };
static const wchar_t dotSemicolonEntityValue[] = { 0x002D9 };
static const wchar_t doteqSemicolonEntityValue[] = { 0x02250 };
static const wchar_t doteqdotSemicolonEntityValue[] = { 0x02251 };
static const wchar_t dotminusSemicolonEntityValue[] = { 0x02238 };
static const wchar_t dotplusSemicolonEntityValue[] = { 0x02214 };
static const wchar_t dotsquareSemicolonEntityValue[] = { 0x022A1 };
static const wchar_t doublebarwedgeSemicolonEntityValue[] = { 0x02306 };
static const wchar_t downarrowSemicolonEntityValue[] = { 0x02193 };
static const wchar_t downdownarrowsSemicolonEntityValue[] = { 0x021CA };
static const wchar_t downharpoonleftSemicolonEntityValue[] = { 0x021C3 };
static const wchar_t downharpoonrightSemicolonEntityValue[] = { 0x021C2 };
static const wchar_t drbkarowSemicolonEntityValue[] = { 0x02910 };
static const wchar_t drcornSemicolonEntityValue[] = { 0x0231F };
static const wchar_t drcropSemicolonEntityValue[] = { 0x0230C };
static const wchar_t dscrSemicolonEntityValue[] = { 0x1D4B9 };
static const wchar_t dscySemicolonEntityValue[] = { 0x00455 };
static const wchar_t dsolSemicolonEntityValue[] = { 0x029F6 };
static const wchar_t dstrokSemicolonEntityValue[] = { 0x00111 };
static const wchar_t dtdotSemicolonEntityValue[] = { 0x022F1 };
static const wchar_t dtriSemicolonEntityValue[] = { 0x025BF };
static const wchar_t dtrifSemicolonEntityValue[] = { 0x025BE };
static const wchar_t duarrSemicolonEntityValue[] = { 0x021F5 };
static const wchar_t duharSemicolonEntityValue[] = { 0x0296F };
static const wchar_t dwangleSemicolonEntityValue[] = { 0x029A6 };
static const wchar_t dzcySemicolonEntityValue[] = { 0x0045F };
static const wchar_t dzigrarrSemicolonEntityValue[] = { 0x027FF };
static const wchar_t eDDotSemicolonEntityValue[] = { 0x02A77 };
static const wchar_t eDotSemicolonEntityValue[] = { 0x02251 };
static const wchar_t eacuteEntityValue[] = { 0x000E9 };
static const wchar_t eacuteSemicolonEntityValue[] = { 0x000E9 };
static const wchar_t easterSemicolonEntityValue[] = { 0x02A6E };
static const wchar_t ecaronSemicolonEntityValue[] = { 0x0011B };
static const wchar_t ecirSemicolonEntityValue[] = { 0x02256 };
static const wchar_t ecircEntityValue[] = { 0x000EA };
static const wchar_t ecircSemicolonEntityValue[] = { 0x000EA };
static const wchar_t ecolonSemicolonEntityValue[] = { 0x02255 };
static const wchar_t ecySemicolonEntityValue[] = { 0x0044D };
static const wchar_t edotSemicolonEntityValue[] = { 0x00117 };
static const wchar_t eeSemicolonEntityValue[] = { 0x02147 };
static const wchar_t efDotSemicolonEntityValue[] = { 0x02252 };
static const wchar_t efrSemicolonEntityValue[] = { 0x1D522 };
static const wchar_t egSemicolonEntityValue[] = { 0x02A9A };
static const wchar_t egraveEntityValue[] = { 0x000E8 };
static const wchar_t egraveSemicolonEntityValue[] = { 0x000E8 };
static const wchar_t egsSemicolonEntityValue[] = { 0x02A96 };
static const wchar_t egsdotSemicolonEntityValue[] = { 0x02A98 };
static const wchar_t elSemicolonEntityValue[] = { 0x02A99 };
static const wchar_t elintersSemicolonEntityValue[] = { 0x023E7 };
static const wchar_t ellSemicolonEntityValue[] = { 0x02113 };
static const wchar_t elsSemicolonEntityValue[] = { 0x02A95 };
static const wchar_t elsdotSemicolonEntityValue[] = { 0x02A97 };
static const wchar_t emacrSemicolonEntityValue[] = { 0x00113 };
static const wchar_t emptySemicolonEntityValue[] = { 0x02205 };
static const wchar_t emptysetSemicolonEntityValue[] = { 0x02205 };
static const wchar_t emptyvSemicolonEntityValue[] = { 0x02205 };
static const wchar_t emsp13SemicolonEntityValue[] = { 0x02004 };
static const wchar_t emsp14SemicolonEntityValue[] = { 0x02005 };
static const wchar_t emspSemicolonEntityValue[] = { 0x02003 };
static const wchar_t engSemicolonEntityValue[] = { 0x0014B };
static const wchar_t enspSemicolonEntityValue[] = { 0x02002 };
static const wchar_t eogonSemicolonEntityValue[] = { 0x00119 };
static const wchar_t eopfSemicolonEntityValue[] = { 0x1D556 };
static const wchar_t eparSemicolonEntityValue[] = { 0x022D5 };
static const wchar_t eparslSemicolonEntityValue[] = { 0x029E3 };
static const wchar_t eplusSemicolonEntityValue[] = { 0x02A71 };
static const wchar_t epsiSemicolonEntityValue[] = { 0x003B5 };
static const wchar_t epsilonSemicolonEntityValue[] = { 0x003B5 };
static const wchar_t epsivSemicolonEntityValue[] = { 0x003F5 };
static const wchar_t eqcircSemicolonEntityValue[] = { 0x02256 };
static const wchar_t eqcolonSemicolonEntityValue[] = { 0x02255 };
static const wchar_t eqsimSemicolonEntityValue[] = { 0x02242 };
static const wchar_t eqslantgtrSemicolonEntityValue[] = { 0x02A96 };
static const wchar_t eqslantlessSemicolonEntityValue[] = { 0x02A95 };
static const wchar_t equalsSemicolonEntityValue[] = { 0x0003D };
static const wchar_t equestSemicolonEntityValue[] = { 0x0225F };
static const wchar_t equivSemicolonEntityValue[] = { 0x02261 };
static const wchar_t equivDDSemicolonEntityValue[] = { 0x02A78 };
static const wchar_t eqvparslSemicolonEntityValue[] = { 0x029E5 };
static const wchar_t erDotSemicolonEntityValue[] = { 0x02253 };
static const wchar_t erarrSemicolonEntityValue[] = { 0x02971 };
static const wchar_t escrSemicolonEntityValue[] = { 0x0212F };
static const wchar_t esdotSemicolonEntityValue[] = { 0x02250 };
static const wchar_t esimSemicolonEntityValue[] = { 0x02242 };
static const wchar_t etaSemicolonEntityValue[] = { 0x003B7 };
static const wchar_t ethEntityValue[] = { 0x000F0 };
static const wchar_t ethSemicolonEntityValue[] = { 0x000F0 };
static const wchar_t eumlEntityValue[] = { 0x000EB };
static const wchar_t eumlSemicolonEntityValue[] = { 0x000EB };
static const wchar_t euroSemicolonEntityValue[] = { 0x020AC };
static const wchar_t exclSemicolonEntityValue[] = { 0x00021 };
static const wchar_t existSemicolonEntityValue[] = { 0x02203 };
static const wchar_t expectationSemicolonEntityValue[] = { 0x02130 };
static const wchar_t exponentialeSemicolonEntityValue[] = { 0x02147 };
static const wchar_t fallingdotseqSemicolonEntityValue[] = { 0x02252 };
static const wchar_t fcySemicolonEntityValue[] = { 0x00444 };
static const wchar_t femaleSemicolonEntityValue[] = { 0x02640 };
static const wchar_t ffiligSemicolonEntityValue[] = { 0x0FB03 };
static const wchar_t ffligSemicolonEntityValue[] = { 0x0FB00 };
static const wchar_t fflligSemicolonEntityValue[] = { 0x0FB04 };
static const wchar_t ffrSemicolonEntityValue[] = { 0x1D523 };
static const wchar_t filigSemicolonEntityValue[] = { 0x0FB01 };
static const wchar_t fjligSemicolonEntityValue[] = { 0x00066, 0x0006A };
static const wchar_t flatSemicolonEntityValue[] = { 0x0266D };
static const wchar_t flligSemicolonEntityValue[] = { 0x0FB02 };
static const wchar_t fltnsSemicolonEntityValue[] = { 0x025B1 };
static const wchar_t fnofSemicolonEntityValue[] = { 0x00192 };
static const wchar_t fopfSemicolonEntityValue[] = { 0x1D557 };
static const wchar_t forallSemicolonEntityValue[] = { 0x02200 };
static const wchar_t forkSemicolonEntityValue[] = { 0x022D4 };
static const wchar_t forkvSemicolonEntityValue[] = { 0x02AD9 };
static const wchar_t fpartintSemicolonEntityValue[] = { 0x02A0D };
static const wchar_t frac12EntityValue[] = { 0x000BD };
static const wchar_t frac12SemicolonEntityValue[] = { 0x000BD };
static const wchar_t frac13SemicolonEntityValue[] = { 0x02153 };
static const wchar_t frac14EntityValue[] = { 0x000BC };
static const wchar_t frac14SemicolonEntityValue[] = { 0x000BC };
static const wchar_t frac15SemicolonEntityValue[] = { 0x02155 };
static const wchar_t frac16SemicolonEntityValue[] = { 0x02159 };
static const wchar_t frac18SemicolonEntityValue[] = { 0x0215B };
static const wchar_t frac23SemicolonEntityValue[] = { 0x02154 };
static const wchar_t frac25SemicolonEntityValue[] = { 0x02156 };
static const wchar_t frac34EntityValue[] = { 0x000BE };
static const wchar_t frac34SemicolonEntityValue[] = { 0x000BE };
static const wchar_t frac35SemicolonEntityValue[] = { 0x02157 };
static const wchar_t frac38SemicolonEntityValue[] = { 0x0215C };
static const wchar_t frac45SemicolonEntityValue[] = { 0x02158 };
static const wchar_t frac56SemicolonEntityValue[] = { 0x0215A };
static const wchar_t frac58SemicolonEntityValue[] = { 0x0215D };
static const wchar_t frac78SemicolonEntityValue[] = { 0x0215E };
static const wchar_t fraslSemicolonEntityValue[] = { 0x02044 };
static const wchar_t frownSemicolonEntityValue[] = { 0x02322 };
static const wchar_t fscrSemicolonEntityValue[] = { 0x1D4BB };
static const wchar_t gESemicolonEntityValue[] = { 0x02267 };
static const wchar_t gElSemicolonEntityValue[] = { 0x02A8C };
static const wchar_t gacuteSemicolonEntityValue[] = { 0x001F5 };
static const wchar_t gammaSemicolonEntityValue[] = { 0x003B3 };
static const wchar_t gammadSemicolonEntityValue[] = { 0x003DD };
static const wchar_t gapSemicolonEntityValue[] = { 0x02A86 };
static const wchar_t gbreveSemicolonEntityValue[] = { 0x0011F };
static const wchar_t gcircSemicolonEntityValue[] = { 0x0011D };
static const wchar_t gcySemicolonEntityValue[] = { 0x00433 };
static const wchar_t gdotSemicolonEntityValue[] = { 0x00121 };
static const wchar_t geSemicolonEntityValue[] = { 0x02265 };
static const wchar_t gelSemicolonEntityValue[] = { 0x022DB };
static const wchar_t geqSemicolonEntityValue[] = { 0x02265 };
static const wchar_t geqqSemicolonEntityValue[] = { 0x02267 };
static const wchar_t geqslantSemicolonEntityValue[] = { 0x02A7E };
static const wchar_t gesSemicolonEntityValue[] = { 0x02A7E };
static const wchar_t gesccSemicolonEntityValue[] = { 0x02AA9 };
static const wchar_t gesdotSemicolonEntityValue[] = { 0x02A80 };
static const wchar_t gesdotoSemicolonEntityValue[] = { 0x02A82 };
static const wchar_t gesdotolSemicolonEntityValue[] = { 0x02A84 };
static const wchar_t geslSemicolonEntityValue[] = { 0x022DB, 0x0FE00 };
static const wchar_t geslesSemicolonEntityValue[] = { 0x02A94 };
static const wchar_t gfrSemicolonEntityValue[] = { 0x1D524 };
static const wchar_t ggSemicolonEntityValue[] = { 0x0226B };
static const wchar_t gggSemicolonEntityValue[] = { 0x022D9 };
static const wchar_t gimelSemicolonEntityValue[] = { 0x02137 };
static const wchar_t gjcySemicolonEntityValue[] = { 0x00453 };
static const wchar_t glSemicolonEntityValue[] = { 0x02277 };
static const wchar_t glESemicolonEntityValue[] = { 0x02A92 };
static const wchar_t glaSemicolonEntityValue[] = { 0x02AA5 };
static const wchar_t gljSemicolonEntityValue[] = { 0x02AA4 };
static const wchar_t gnESemicolonEntityValue[] = { 0x02269 };
static const wchar_t gnapSemicolonEntityValue[] = { 0x02A8A };
static const wchar_t gnapproxSemicolonEntityValue[] = { 0x02A8A };
static const wchar_t gneSemicolonEntityValue[] = { 0x02A88 };
static const wchar_t gneqSemicolonEntityValue[] = { 0x02A88 };
static const wchar_t gneqqSemicolonEntityValue[] = { 0x02269 };
static const wchar_t gnsimSemicolonEntityValue[] = { 0x022E7 };
static const wchar_t gopfSemicolonEntityValue[] = { 0x1D558 };
static const wchar_t graveSemicolonEntityValue[] = { 0x00060 };
static const wchar_t gscrSemicolonEntityValue[] = { 0x0210A };
static const wchar_t gsimSemicolonEntityValue[] = { 0x02273 };
static const wchar_t gsimeSemicolonEntityValue[] = { 0x02A8E };
static const wchar_t gsimlSemicolonEntityValue[] = { 0x02A90 };
static const wchar_t gtEntityValue[] = { 0x0003E };
static const wchar_t gtSemicolonEntityValue[] = { 0x0003E };
static const wchar_t gtccSemicolonEntityValue[] = { 0x02AA7 };
static const wchar_t gtcirSemicolonEntityValue[] = { 0x02A7A };
static const wchar_t gtdotSemicolonEntityValue[] = { 0x022D7 };
static const wchar_t gtlParSemicolonEntityValue[] = { 0x02995 };
static const wchar_t gtquestSemicolonEntityValue[] = { 0x02A7C };
static const wchar_t gtrapproxSemicolonEntityValue[] = { 0x02A86 };
static const wchar_t gtrarrSemicolonEntityValue[] = { 0x02978 };
static const wchar_t gtrdotSemicolonEntityValue[] = { 0x022D7 };
static const wchar_t gtreqlessSemicolonEntityValue[] = { 0x022DB };
static const wchar_t gtreqqlessSemicolonEntityValue[] = { 0x02A8C };
static const wchar_t gtrlessSemicolonEntityValue[] = { 0x02277 };
static const wchar_t gtrsimSemicolonEntityValue[] = { 0x02273 };
static const wchar_t gvertneqqSemicolonEntityValue[] = { 0x02269, 0x0FE00 };
static const wchar_t gvnESemicolonEntityValue[] = { 0x02269, 0x0FE00 };
static const wchar_t hArrSemicolonEntityValue[] = { 0x021D4 };
static const wchar_t hairspSemicolonEntityValue[] = { 0x0200A };
static const wchar_t halfSemicolonEntityValue[] = { 0x000BD };
static const wchar_t hamiltSemicolonEntityValue[] = { 0x0210B };
static const wchar_t hardcySemicolonEntityValue[] = { 0x0044A };
static const wchar_t harrSemicolonEntityValue[] = { 0x02194 };
static const wchar_t harrcirSemicolonEntityValue[] = { 0x02948 };
static const wchar_t harrwSemicolonEntityValue[] = { 0x021AD };
static const wchar_t hbarSemicolonEntityValue[] = { 0x0210F };
static const wchar_t hcircSemicolonEntityValue[] = { 0x00125 };
static const wchar_t heartsSemicolonEntityValue[] = { 0x02665 };
static const wchar_t heartsuitSemicolonEntityValue[] = { 0x02665 };
static const wchar_t hellipSemicolonEntityValue[] = { 0x02026 };
static const wchar_t herconSemicolonEntityValue[] = { 0x022B9 };
static const wchar_t hfrSemicolonEntityValue[] = { 0x1D525 };
static const wchar_t hksearowSemicolonEntityValue[] = { 0x02925 };
static const wchar_t hkswarowSemicolonEntityValue[] = { 0x02926 };
static const wchar_t hoarrSemicolonEntityValue[] = { 0x021FF };
static const wchar_t homthtSemicolonEntityValue[] = { 0x0223B };
static const wchar_t hookleftarrowSemicolonEntityValue[] = { 0x021A9 };
static const wchar_t hookrightarrowSemicolonEntityValue[] = { 0x021AA };
static const wchar_t hopfSemicolonEntityValue[] = { 0x1D559 };
static const wchar_t horbarSemicolonEntityValue[] = { 0x02015 };
static const wchar_t hscrSemicolonEntityValue[] = { 0x1D4BD };
static const wchar_t hslashSemicolonEntityValue[] = { 0x0210F };
static const wchar_t hstrokSemicolonEntityValue[] = { 0x00127 };
static const wchar_t hybullSemicolonEntityValue[] = { 0x02043 };
static const wchar_t hyphenSemicolonEntityValue[] = { 0x02010 };
static const wchar_t iacuteEntityValue[] = { 0x000ED };
static const wchar_t iacuteSemicolonEntityValue[] = { 0x000ED };
static const wchar_t icSemicolonEntityValue[] = { 0x02063 };
static const wchar_t icircEntityValue[] = { 0x000EE };
static const wchar_t icircSemicolonEntityValue[] = { 0x000EE };
static const wchar_t icySemicolonEntityValue[] = { 0x00438 };
static const wchar_t iecySemicolonEntityValue[] = { 0x00435 };
static const wchar_t iexclEntityValue[] = { 0x000A1 };
static const wchar_t iexclSemicolonEntityValue[] = { 0x000A1 };
static const wchar_t iffSemicolonEntityValue[] = { 0x021D4 };
static const wchar_t ifrSemicolonEntityValue[] = { 0x1D526 };
static const wchar_t igraveEntityValue[] = { 0x000EC };
static const wchar_t igraveSemicolonEntityValue[] = { 0x000EC };
static const wchar_t iiSemicolonEntityValue[] = { 0x02148 };
static const wchar_t iiiintSemicolonEntityValue[] = { 0x02A0C };
static const wchar_t iiintSemicolonEntityValue[] = { 0x0222D };
static const wchar_t iinfinSemicolonEntityValue[] = { 0x029DC };
static const wchar_t iiotaSemicolonEntityValue[] = { 0x02129 };
static const wchar_t ijligSemicolonEntityValue[] = { 0x00133 };
static const wchar_t imacrSemicolonEntityValue[] = { 0x0012B };
static const wchar_t imageSemicolonEntityValue[] = { 0x02111 };
static const wchar_t imaglineSemicolonEntityValue[] = { 0x02110 };
static const wchar_t imagpartSemicolonEntityValue[] = { 0x02111 };
static const wchar_t imathSemicolonEntityValue[] = { 0x00131 };
static const wchar_t imofSemicolonEntityValue[] = { 0x022B7 };
static const wchar_t impedSemicolonEntityValue[] = { 0x001B5 };
static const wchar_t inSemicolonEntityValue[] = { 0x02208 };
static const wchar_t incareSemicolonEntityValue[] = { 0x02105 };
static const wchar_t infinSemicolonEntityValue[] = { 0x0221E };
static const wchar_t infintieSemicolonEntityValue[] = { 0x029DD };
static const wchar_t inodotSemicolonEntityValue[] = { 0x00131 };
static const wchar_t intSemicolonEntityValue[] = { 0x0222B };
static const wchar_t intcalSemicolonEntityValue[] = { 0x022BA };
static const wchar_t integersSemicolonEntityValue[] = { 0x02124 };
static const wchar_t intercalSemicolonEntityValue[] = { 0x022BA };
static const wchar_t intlarhkSemicolonEntityValue[] = { 0x02A17 };
static const wchar_t intprodSemicolonEntityValue[] = { 0x02A3C };
static const wchar_t iocySemicolonEntityValue[] = { 0x00451 };
static const wchar_t iogonSemicolonEntityValue[] = { 0x0012F };
static const wchar_t iopfSemicolonEntityValue[] = { 0x1D55A };
static const wchar_t iotaSemicolonEntityValue[] = { 0x003B9 };
static const wchar_t iprodSemicolonEntityValue[] = { 0x02A3C };
static const wchar_t iquestEntityValue[] = { 0x000BF };
static const wchar_t iquestSemicolonEntityValue[] = { 0x000BF };
static const wchar_t iscrSemicolonEntityValue[] = { 0x1D4BE };
static const wchar_t isinSemicolonEntityValue[] = { 0x02208 };
static const wchar_t isinESemicolonEntityValue[] = { 0x022F9 };
static const wchar_t isindotSemicolonEntityValue[] = { 0x022F5 };
static const wchar_t isinsSemicolonEntityValue[] = { 0x022F4 };
static const wchar_t isinsvSemicolonEntityValue[] = { 0x022F3 };
static const wchar_t isinvSemicolonEntityValue[] = { 0x02208 };
static const wchar_t itSemicolonEntityValue[] = { 0x02062 };
static const wchar_t itildeSemicolonEntityValue[] = { 0x00129 };
static const wchar_t iukcySemicolonEntityValue[] = { 0x00456 };
static const wchar_t iumlEntityValue[] = { 0x000EF };
static const wchar_t iumlSemicolonEntityValue[] = { 0x000EF };
static const wchar_t jcircSemicolonEntityValue[] = { 0x00135 };
static const wchar_t jcySemicolonEntityValue[] = { 0x00439 };
static const wchar_t jfrSemicolonEntityValue[] = { 0x1D527 };
static const wchar_t jmathSemicolonEntityValue[] = { 0x00237 };
static const wchar_t jopfSemicolonEntityValue[] = { 0x1D55B };
static const wchar_t jscrSemicolonEntityValue[] = { 0x1D4BF };
static const wchar_t jsercySemicolonEntityValue[] = { 0x00458 };
static const wchar_t jukcySemicolonEntityValue[] = { 0x00454 };
static const wchar_t kappaSemicolonEntityValue[] = { 0x003BA };
static const wchar_t kappavSemicolonEntityValue[] = { 0x003F0 };
static const wchar_t kcedilSemicolonEntityValue[] = { 0x00137 };
static const wchar_t kcySemicolonEntityValue[] = { 0x0043A };
static const wchar_t kfrSemicolonEntityValue[] = { 0x1D528 };
static const wchar_t kgreenSemicolonEntityValue[] = { 0x00138 };
static const wchar_t khcySemicolonEntityValue[] = { 0x00445 };
static const wchar_t kjcySemicolonEntityValue[] = { 0x0045C };
static const wchar_t kopfSemicolonEntityValue[] = { 0x1D55C };
static const wchar_t kscrSemicolonEntityValue[] = { 0x1D4C0 };
static const wchar_t lAarrSemicolonEntityValue[] = { 0x021DA };
static const wchar_t lArrSemicolonEntityValue[] = { 0x021D0 };
static const wchar_t lAtailSemicolonEntityValue[] = { 0x0291B };
static const wchar_t lBarrSemicolonEntityValue[] = { 0x0290E };
static const wchar_t lESemicolonEntityValue[] = { 0x02266 };
static const wchar_t lEgSemicolonEntityValue[] = { 0x02A8B };
static const wchar_t lHarSemicolonEntityValue[] = { 0x02962 };
static const wchar_t lacuteSemicolonEntityValue[] = { 0x0013A };
static const wchar_t laemptyvSemicolonEntityValue[] = { 0x029B4 };
static const wchar_t lagranSemicolonEntityValue[] = { 0x02112 };
static const wchar_t lambdaSemicolonEntityValue[] = { 0x003BB };
static const wchar_t langSemicolonEntityValue[] = { 0x027E8 };
static const wchar_t langdSemicolonEntityValue[] = { 0x02991 };
static const wchar_t langleSemicolonEntityValue[] = { 0x027E8 };
static const wchar_t lapSemicolonEntityValue[] = { 0x02A85 };
static const wchar_t laquoEntityValue[] = { 0x000AB };
static const wchar_t laquoSemicolonEntityValue[] = { 0x000AB };
static const wchar_t larrSemicolonEntityValue[] = { 0x02190 };
static const wchar_t larrbSemicolonEntityValue[] = { 0x021E4 };
static const wchar_t larrbfsSemicolonEntityValue[] = { 0x0291F };
static const wchar_t larrfsSemicolonEntityValue[] = { 0x0291D };
static const wchar_t larrhkSemicolonEntityValue[] = { 0x021A9 };
static const wchar_t larrlpSemicolonEntityValue[] = { 0x021AB };
static const wchar_t larrplSemicolonEntityValue[] = { 0x02939 };
static const wchar_t larrsimSemicolonEntityValue[] = { 0x02973 };
static const wchar_t larrtlSemicolonEntityValue[] = { 0x021A2 };
static const wchar_t latSemicolonEntityValue[] = { 0x02AAB };
static const wchar_t latailSemicolonEntityValue[] = { 0x02919 };
static const wchar_t lateSemicolonEntityValue[] = { 0x02AAD };
static const wchar_t latesSemicolonEntityValue[] = { 0x02AAD, 0x0FE00 };
static const wchar_t lbarrSemicolonEntityValue[] = { 0x0290C };
static const wchar_t lbbrkSemicolonEntityValue[] = { 0x02772 };
static const wchar_t lbraceSemicolonEntityValue[] = { 0x0007B };
static const wchar_t lbrackSemicolonEntityValue[] = { 0x0005B };
static const wchar_t lbrkeSemicolonEntityValue[] = { 0x0298B };
static const wchar_t lbrksldSemicolonEntityValue[] = { 0x0298F };
static const wchar_t lbrksluSemicolonEntityValue[] = { 0x0298D };
static const wchar_t lcaronSemicolonEntityValue[] = { 0x0013E };
static const wchar_t lcedilSemicolonEntityValue[] = { 0x0013C };
static const wchar_t lceilSemicolonEntityValue[] = { 0x02308 };
static const wchar_t lcubSemicolonEntityValue[] = { 0x0007B };
static const wchar_t lcySemicolonEntityValue[] = { 0x0043B };
static const wchar_t ldcaSemicolonEntityValue[] = { 0x02936 };
static const wchar_t ldquoSemicolonEntityValue[] = { 0x0201C };
static const wchar_t ldquorSemicolonEntityValue[] = { 0x0201E };
static const wchar_t ldrdharSemicolonEntityValue[] = { 0x02967 };
static const wchar_t ldrusharSemicolonEntityValue[] = { 0x0294B };
static const wchar_t ldshSemicolonEntityValue[] = { 0x021B2 };
static const wchar_t leSemicolonEntityValue[] = { 0x02264 };
static const wchar_t leftarrowSemicolonEntityValue[] = { 0x02190 };
static const wchar_t leftarrowtailSemicolonEntityValue[] = { 0x021A2 };
static const wchar_t leftharpoondownSemicolonEntityValue[] = { 0x021BD };
static const wchar_t leftharpoonupSemicolonEntityValue[] = { 0x021BC };
static const wchar_t leftleftarrowsSemicolonEntityValue[] = { 0x021C7 };
static const wchar_t leftrightarrowSemicolonEntityValue[] = { 0x02194 };
static const wchar_t leftrightarrowsSemicolonEntityValue[] = { 0x021C6 };
static const wchar_t leftrightharpoonsSemicolonEntityValue[] = { 0x021CB };
static const wchar_t leftrightsquigarrowSemicolonEntityValue[] = { 0x021AD };
static const wchar_t leftthreetimesSemicolonEntityValue[] = { 0x022CB };
static const wchar_t legSemicolonEntityValue[] = { 0x022DA };
static const wchar_t leqSemicolonEntityValue[] = { 0x02264 };
static const wchar_t leqqSemicolonEntityValue[] = { 0x02266 };
static const wchar_t leqslantSemicolonEntityValue[] = { 0x02A7D };
static const wchar_t lesSemicolonEntityValue[] = { 0x02A7D };
static const wchar_t lesccSemicolonEntityValue[] = { 0x02AA8 };
static const wchar_t lesdotSemicolonEntityValue[] = { 0x02A7F };
static const wchar_t lesdotoSemicolonEntityValue[] = { 0x02A81 };
static const wchar_t lesdotorSemicolonEntityValue[] = { 0x02A83 };
static const wchar_t lesgSemicolonEntityValue[] = { 0x022DA, 0x0FE00 };
static const wchar_t lesgesSemicolonEntityValue[] = { 0x02A93 };
static const wchar_t lessapproxSemicolonEntityValue[] = { 0x02A85 };
static const wchar_t lessdotSemicolonEntityValue[] = { 0x022D6 };
static const wchar_t lesseqgtrSemicolonEntityValue[] = { 0x022DA };
static const wchar_t lesseqqgtrSemicolonEntityValue[] = { 0x02A8B };
static const wchar_t lessgtrSemicolonEntityValue[] = { 0x02276 };
static const wchar_t lesssimSemicolonEntityValue[] = { 0x02272 };
static const wchar_t lfishtSemicolonEntityValue[] = { 0x0297C };
static const wchar_t lfloorSemicolonEntityValue[] = { 0x0230A };
static const wchar_t lfrSemicolonEntityValue[] = { 0x1D529 };
static const wchar_t lgSemicolonEntityValue[] = { 0x02276 };
static const wchar_t lgESemicolonEntityValue[] = { 0x02A91 };
static const wchar_t lhardSemicolonEntityValue[] = { 0x021BD };
static const wchar_t lharuSemicolonEntityValue[] = { 0x021BC };
static const wchar_t lharulSemicolonEntityValue[] = { 0x0296A };
static const wchar_t lhblkSemicolonEntityValue[] = { 0x02584 };
static const wchar_t ljcySemicolonEntityValue[] = { 0x00459 };
static const wchar_t llSemicolonEntityValue[] = { 0x0226A };
static const wchar_t llarrSemicolonEntityValue[] = { 0x021C7 };
static const wchar_t llcornerSemicolonEntityValue[] = { 0x0231E };
static const wchar_t llhardSemicolonEntityValue[] = { 0x0296B };
static const wchar_t lltriSemicolonEntityValue[] = { 0x025FA };
static const wchar_t lmidotSemicolonEntityValue[] = { 0x00140 };
static const wchar_t lmoustSemicolonEntityValue[] = { 0x023B0 };
static const wchar_t lmoustacheSemicolonEntityValue[] = { 0x023B0 };
static const wchar_t lnESemicolonEntityValue[] = { 0x02268 };
static const wchar_t lnapSemicolonEntityValue[] = { 0x02A89 };
static const wchar_t lnapproxSemicolonEntityValue[] = { 0x02A89 };
static const wchar_t lneSemicolonEntityValue[] = { 0x02A87 };
static const wchar_t lneqSemicolonEntityValue[] = { 0x02A87 };
static const wchar_t lneqqSemicolonEntityValue[] = { 0x02268 };
static const wchar_t lnsimSemicolonEntityValue[] = { 0x022E6 };
static const wchar_t loangSemicolonEntityValue[] = { 0x027EC };
static const wchar_t loarrSemicolonEntityValue[] = { 0x021FD };
static const wchar_t lobrkSemicolonEntityValue[] = { 0x027E6 };
static const wchar_t longleftarrowSemicolonEntityValue[] = { 0x027F5 };
static const wchar_t longleftrightarrowSemicolonEntityValue[] = { 0x027F7 };
static const wchar_t longmapstoSemicolonEntityValue[] = { 0x027FC };
static const wchar_t longrightarrowSemicolonEntityValue[] = { 0x027F6 };
static const wchar_t looparrowleftSemicolonEntityValue[] = { 0x021AB };
static const wchar_t looparrowrightSemicolonEntityValue[] = { 0x021AC };
static const wchar_t loparSemicolonEntityValue[] = { 0x02985 };
static const wchar_t lopfSemicolonEntityValue[] = { 0x1D55D };
static const wchar_t loplusSemicolonEntityValue[] = { 0x02A2D };
static const wchar_t lotimesSemicolonEntityValue[] = { 0x02A34 };
static const wchar_t lowastSemicolonEntityValue[] = { 0x02217 };
static const wchar_t lowbarSemicolonEntityValue[] = { 0x0005F };
static const wchar_t lozSemicolonEntityValue[] = { 0x025CA };
static const wchar_t lozengeSemicolonEntityValue[] = { 0x025CA };
static const wchar_t lozfSemicolonEntityValue[] = { 0x029EB };
static const wchar_t lparSemicolonEntityValue[] = { 0x00028 };
static const wchar_t lparltSemicolonEntityValue[] = { 0x02993 };
static const wchar_t lrarrSemicolonEntityValue[] = { 0x021C6 };
static const wchar_t lrcornerSemicolonEntityValue[] = { 0x0231F };
static const wchar_t lrharSemicolonEntityValue[] = { 0x021CB };
static const wchar_t lrhardSemicolonEntityValue[] = { 0x0296D };
static const wchar_t lrmSemicolonEntityValue[] = { 0x0200E };
static const wchar_t lrtriSemicolonEntityValue[] = { 0x022BF };
static const wchar_t lsaquoSemicolonEntityValue[] = { 0x02039 };
static const wchar_t lscrSemicolonEntityValue[] = { 0x1D4C1 };
static const wchar_t lshSemicolonEntityValue[] = { 0x021B0 };
static const wchar_t lsimSemicolonEntityValue[] = { 0x02272 };
static const wchar_t lsimeSemicolonEntityValue[] = { 0x02A8D };
static const wchar_t lsimgSemicolonEntityValue[] = { 0x02A8F };
static const wchar_t lsqbSemicolonEntityValue[] = { 0x0005B };
static const wchar_t lsquoSemicolonEntityValue[] = { 0x02018 };
static const wchar_t lsquorSemicolonEntityValue[] = { 0x0201A };
static const wchar_t lstrokSemicolonEntityValue[] = { 0x00142 };
static const wchar_t ltEntityValue[] = { 0x0003C };
static const wchar_t ltSemicolonEntityValue[] = { 0x0003C };
static const wchar_t ltccSemicolonEntityValue[] = { 0x02AA6 };
static const wchar_t ltcirSemicolonEntityValue[] = { 0x02A79 };
static const wchar_t ltdotSemicolonEntityValue[] = { 0x022D6 };
static const wchar_t lthreeSemicolonEntityValue[] = { 0x022CB };
static const wchar_t ltimesSemicolonEntityValue[] = { 0x022C9 };
static const wchar_t ltlarrSemicolonEntityValue[] = { 0x02976 };
static const wchar_t ltquestSemicolonEntityValue[] = { 0x02A7B };
static const wchar_t ltrParSemicolonEntityValue[] = { 0x02996 };
static const wchar_t ltriSemicolonEntityValue[] = { 0x025C3 };
static const wchar_t ltrieSemicolonEntityValue[] = { 0x022B4 };
static const wchar_t ltrifSemicolonEntityValue[] = { 0x025C2 };
static const wchar_t lurdsharSemicolonEntityValue[] = { 0x0294A };
static const wchar_t luruharSemicolonEntityValue[] = { 0x02966 };
static const wchar_t lvertneqqSemicolonEntityValue[] = { 0x02268, 0x0FE00 };
static const wchar_t lvnESemicolonEntityValue[] = { 0x02268, 0x0FE00 };
static const wchar_t mDDotSemicolonEntityValue[] = { 0x0223A };
static const wchar_t macrEntityValue[] = { 0x000AF };
static const wchar_t macrSemicolonEntityValue[] = { 0x000AF };
static const wchar_t maleSemicolonEntityValue[] = { 0x02642 };
static const wchar_t maltSemicolonEntityValue[] = { 0x02720 };
static const wchar_t malteseSemicolonEntityValue[] = { 0x02720 };
static const wchar_t mapSemicolonEntityValue[] = { 0x021A6 };
static const wchar_t mapstoSemicolonEntityValue[] = { 0x021A6 };
static const wchar_t mapstodownSemicolonEntityValue[] = { 0x021A7 };
static const wchar_t mapstoleftSemicolonEntityValue[] = { 0x021A4 };
static const wchar_t mapstoupSemicolonEntityValue[] = { 0x021A5 };
static const wchar_t markerSemicolonEntityValue[] = { 0x025AE };
static const wchar_t mcommaSemicolonEntityValue[] = { 0x02A29 };
static const wchar_t mcySemicolonEntityValue[] = { 0x0043C };
static const wchar_t mdashSemicolonEntityValue[] = { 0x02014 };
static const wchar_t measuredangleSemicolonEntityValue[] = { 0x02221 };
static const wchar_t mfrSemicolonEntityValue[] = { 0x1D52A };
static const wchar_t mhoSemicolonEntityValue[] = { 0x02127 };
static const wchar_t microEntityValue[] = { 0x000B5 };
static const wchar_t microSemicolonEntityValue[] = { 0x000B5 };
static const wchar_t midSemicolonEntityValue[] = { 0x02223 };
static const wchar_t midastSemicolonEntityValue[] = { 0x0002A };
static const wchar_t midcirSemicolonEntityValue[] = { 0x02AF0 };
static const wchar_t middotEntityValue[] = { 0x000B7 };
static const wchar_t middotSemicolonEntityValue[] = { 0x000B7 };
static const wchar_t minusSemicolonEntityValue[] = { 0x02212 };
static const wchar_t minusbSemicolonEntityValue[] = { 0x0229F };
static const wchar_t minusdSemicolonEntityValue[] = { 0x02238 };
static const wchar_t minusduSemicolonEntityValue[] = { 0x02A2A };
static const wchar_t mlcpSemicolonEntityValue[] = { 0x02ADB };
static const wchar_t mldrSemicolonEntityValue[] = { 0x02026 };
static const wchar_t mnplusSemicolonEntityValue[] = { 0x02213 };
static const wchar_t modelsSemicolonEntityValue[] = { 0x022A7 };
static const wchar_t mopfSemicolonEntityValue[] = { 0x1D55E };
static const wchar_t mpSemicolonEntityValue[] = { 0x02213 };
static const wchar_t mscrSemicolonEntityValue[] = { 0x1D4C2 };
static const wchar_t mstposSemicolonEntityValue[] = { 0x0223E };
static const wchar_t muSemicolonEntityValue[] = { 0x003BC };
static const wchar_t multimapSemicolonEntityValue[] = { 0x022B8 };
static const wchar_t mumapSemicolonEntityValue[] = { 0x022B8 };
static const wchar_t nGgSemicolonEntityValue[] = { 0x022D9, 0x00338 };
static const wchar_t nGtSemicolonEntityValue[] = { 0x0226B, 0x020D2 };
static const wchar_t nGtvSemicolonEntityValue[] = { 0x0226B, 0x00338 };
static const wchar_t nLeftarrowSemicolonEntityValue[] = { 0x021CD };
static const wchar_t nLeftrightarrowSemicolonEntityValue[] = { 0x021CE };
static const wchar_t nLlSemicolonEntityValue[] = { 0x022D8, 0x00338 };
static const wchar_t nLtSemicolonEntityValue[] = { 0x0226A, 0x020D2 };
static const wchar_t nLtvSemicolonEntityValue[] = { 0x0226A, 0x00338 };
static const wchar_t nRightarrowSemicolonEntityValue[] = { 0x021CF };
static const wchar_t nVDashSemicolonEntityValue[] = { 0x022AF };
static const wchar_t nVdashSemicolonEntityValue[] = { 0x022AE };
static const wchar_t nablaSemicolonEntityValue[] = { 0x02207 };
static const wchar_t nacuteSemicolonEntityValue[] = { 0x00144 };
static const wchar_t nangSemicolonEntityValue[] = { 0x02220, 0x020D2 };
static const wchar_t napSemicolonEntityValue[] = { 0x02249 };
static const wchar_t napESemicolonEntityValue[] = { 0x02A70, 0x00338 };
static const wchar_t napidSemicolonEntityValue[] = { 0x0224B, 0x00338 };
static const wchar_t naposSemicolonEntityValue[] = { 0x00149 };
static const wchar_t napproxSemicolonEntityValue[] = { 0x02249 };
static const wchar_t naturSemicolonEntityValue[] = { 0x0266E };
static const wchar_t naturalSemicolonEntityValue[] = { 0x0266E };
static const wchar_t naturalsSemicolonEntityValue[] = { 0x02115 };
static const wchar_t nbspEntityValue[] = { 0x000A0 };
static const wchar_t nbspSemicolonEntityValue[] = { 0x000A0 };
static const wchar_t nbumpSemicolonEntityValue[] = { 0x0224E, 0x00338 };
static const wchar_t nbumpeSemicolonEntityValue[] = { 0x0224F, 0x00338 };
static const wchar_t ncapSemicolonEntityValue[] = { 0x02A43 };
static const wchar_t ncaronSemicolonEntityValue[] = { 0x00148 };
static const wchar_t ncedilSemicolonEntityValue[] = { 0x00146 };
static const wchar_t ncongSemicolonEntityValue[] = { 0x02247 };
static const wchar_t ncongdotSemicolonEntityValue[] = { 0x02A6D, 0x00338 };
static const wchar_t ncupSemicolonEntityValue[] = { 0x02A42 };
static const wchar_t ncySemicolonEntityValue[] = { 0x0043D };
static const wchar_t ndashSemicolonEntityValue[] = { 0x02013 };
static const wchar_t neSemicolonEntityValue[] = { 0x02260 };
static const wchar_t neArrSemicolonEntityValue[] = { 0x021D7 };
static const wchar_t nearhkSemicolonEntityValue[] = { 0x02924 };
static const wchar_t nearrSemicolonEntityValue[] = { 0x02197 };
static const wchar_t nearrowSemicolonEntityValue[] = { 0x02197 };
static const wchar_t nedotSemicolonEntityValue[] = { 0x02250, 0x00338 };
static const wchar_t nequivSemicolonEntityValue[] = { 0x02262 };
static const wchar_t nesearSemicolonEntityValue[] = { 0x02928 };
static const wchar_t nesimSemicolonEntityValue[] = { 0x02242, 0x00338 };
static const wchar_t nexistSemicolonEntityValue[] = { 0x02204 };
static const wchar_t nexistsSemicolonEntityValue[] = { 0x02204 };
static const wchar_t nfrSemicolonEntityValue[] = { 0x1D52B };
static const wchar_t ngESemicolonEntityValue[] = { 0x02267, 0x00338 };
static const wchar_t ngeSemicolonEntityValue[] = { 0x02271 };
static const wchar_t ngeqSemicolonEntityValue[] = { 0x02271 };
static const wchar_t ngeqqSemicolonEntityValue[] = { 0x02267, 0x00338 };
static const wchar_t ngeqslantSemicolonEntityValue[] = { 0x02A7E, 0x00338 };
static const wchar_t ngesSemicolonEntityValue[] = { 0x02A7E, 0x00338 };
static const wchar_t ngsimSemicolonEntityValue[] = { 0x02275 };
static const wchar_t ngtSemicolonEntityValue[] = { 0x0226F };
static const wchar_t ngtrSemicolonEntityValue[] = { 0x0226F };
static const wchar_t nhArrSemicolonEntityValue[] = { 0x021CE };
static const wchar_t nharrSemicolonEntityValue[] = { 0x021AE };
static const wchar_t nhparSemicolonEntityValue[] = { 0x02AF2 };
static const wchar_t niSemicolonEntityValue[] = { 0x0220B };
static const wchar_t nisSemicolonEntityValue[] = { 0x022FC };
static const wchar_t nisdSemicolonEntityValue[] = { 0x022FA };
static const wchar_t nivSemicolonEntityValue[] = { 0x0220B };
static const wchar_t njcySemicolonEntityValue[] = { 0x0045A };
static const wchar_t nlArrSemicolonEntityValue[] = { 0x021CD };
static const wchar_t nlESemicolonEntityValue[] = { 0x02266, 0x00338 };
static const wchar_t nlarrSemicolonEntityValue[] = { 0x0219A };
static const wchar_t nldrSemicolonEntityValue[] = { 0x02025 };
static const wchar_t nleSemicolonEntityValue[] = { 0x02270 };
static const wchar_t nleftarrowSemicolonEntityValue[] = { 0x0219A };
static const wchar_t nleftrightarrowSemicolonEntityValue[] = { 0x021AE };
static const wchar_t nleqSemicolonEntityValue[] = { 0x02270 };
static const wchar_t nleqqSemicolonEntityValue[] = { 0x02266, 0x00338 };
static const wchar_t nleqslantSemicolonEntityValue[] = { 0x02A7D, 0x00338 };
static const wchar_t nlesSemicolonEntityValue[] = { 0x02A7D, 0x00338 };
static const wchar_t nlessSemicolonEntityValue[] = { 0x0226E };
static const wchar_t nlsimSemicolonEntityValue[] = { 0x02274 };
static const wchar_t nltSemicolonEntityValue[] = { 0x0226E };
static const wchar_t nltriSemicolonEntityValue[] = { 0x022EA };
static const wchar_t nltrieSemicolonEntityValue[] = { 0x022EC };
static const wchar_t nmidSemicolonEntityValue[] = { 0x02224 };
static const wchar_t nopfSemicolonEntityValue[] = { 0x1D55F };
static const wchar_t notEntityValue[] = { 0x000AC };
static const wchar_t notSemicolonEntityValue[] = { 0x000AC };
static const wchar_t notinSemicolonEntityValue[] = { 0x02209 };
static const wchar_t notinESemicolonEntityValue[] = { 0x022F9, 0x00338 };
static const wchar_t notindotSemicolonEntityValue[] = { 0x022F5, 0x00338 };
static const wchar_t notinvaSemicolonEntityValue[] = { 0x02209 };
static const wchar_t notinvbSemicolonEntityValue[] = { 0x022F7 };
static const wchar_t notinvcSemicolonEntityValue[] = { 0x022F6 };
static const wchar_t notniSemicolonEntityValue[] = { 0x0220C };
static const wchar_t notnivaSemicolonEntityValue[] = { 0x0220C };
static const wchar_t notnivbSemicolonEntityValue[] = { 0x022FE };
static const wchar_t notnivcSemicolonEntityValue[] = { 0x022FD };
static const wchar_t nparSemicolonEntityValue[] = { 0x02226 };
static const wchar_t nparallelSemicolonEntityValue[] = { 0x02226 };
static const wchar_t nparslSemicolonEntityValue[] = { 0x02AFD, 0x020E5 };
static const wchar_t npartSemicolonEntityValue[] = { 0x02202, 0x00338 };
static const wchar_t npolintSemicolonEntityValue[] = { 0x02A14 };
static const wchar_t nprSemicolonEntityValue[] = { 0x02280 };
static const wchar_t nprcueSemicolonEntityValue[] = { 0x022E0 };
static const wchar_t npreSemicolonEntityValue[] = { 0x02AAF, 0x00338 };
static const wchar_t nprecSemicolonEntityValue[] = { 0x02280 };
static const wchar_t npreceqSemicolonEntityValue[] = { 0x02AAF, 0x00338 };
static const wchar_t nrArrSemicolonEntityValue[] = { 0x021CF };
static const wchar_t nrarrSemicolonEntityValue[] = { 0x0219B };
static const wchar_t nrarrcSemicolonEntityValue[] = { 0x02933, 0x00338 };
static const wchar_t nrarrwSemicolonEntityValue[] = { 0x0219D, 0x00338 };
static const wchar_t nrightarrowSemicolonEntityValue[] = { 0x0219B };
static const wchar_t nrtriSemicolonEntityValue[] = { 0x022EB };
static const wchar_t nrtrieSemicolonEntityValue[] = { 0x022ED };
static const wchar_t nscSemicolonEntityValue[] = { 0x02281 };
static const wchar_t nsccueSemicolonEntityValue[] = { 0x022E1 };
static const wchar_t nsceSemicolonEntityValue[] = { 0x02AB0, 0x00338 };
static const wchar_t nscrSemicolonEntityValue[] = { 0x1D4C3 };
static const wchar_t nshortmidSemicolonEntityValue[] = { 0x02224 };
static const wchar_t nshortparallelSemicolonEntityValue[] = { 0x02226 };
static const wchar_t nsimSemicolonEntityValue[] = { 0x02241 };
static const wchar_t nsimeSemicolonEntityValue[] = { 0x02244 };
static const wchar_t nsimeqSemicolonEntityValue[] = { 0x02244 };
static const wchar_t nsmidSemicolonEntityValue[] = { 0x02224 };
static const wchar_t nsparSemicolonEntityValue[] = { 0x02226 };
static const wchar_t nsqsubeSemicolonEntityValue[] = { 0x022E2 };
static const wchar_t nsqsupeSemicolonEntityValue[] = { 0x022E3 };
static const wchar_t nsubSemicolonEntityValue[] = { 0x02284 };
static const wchar_t nsubESemicolonEntityValue[] = { 0x02AC5, 0x00338 };
static const wchar_t nsubeSemicolonEntityValue[] = { 0x02288 };
static const wchar_t nsubsetSemicolonEntityValue[] = { 0x02282, 0x020D2 };
static const wchar_t nsubseteqSemicolonEntityValue[] = { 0x02288 };
static const wchar_t nsubseteqqSemicolonEntityValue[] = { 0x02AC5, 0x00338 };
static const wchar_t nsuccSemicolonEntityValue[] = { 0x02281 };
static const wchar_t nsucceqSemicolonEntityValue[] = { 0x02AB0, 0x00338 };
static const wchar_t nsupSemicolonEntityValue[] = { 0x02285 };
static const wchar_t nsupESemicolonEntityValue[] = { 0x02AC6, 0x00338 };
static const wchar_t nsupeSemicolonEntityValue[] = { 0x02289 };
static const wchar_t nsupsetSemicolonEntityValue[] = { 0x02283, 0x020D2 };
static const wchar_t nsupseteqSemicolonEntityValue[] = { 0x02289 };
static const wchar_t nsupseteqqSemicolonEntityValue[] = { 0x02AC6, 0x00338 };
static const wchar_t ntglSemicolonEntityValue[] = { 0x02279 };
static const wchar_t ntildeEntityValue[] = { 0x000F1 };
static const wchar_t ntildeSemicolonEntityValue[] = { 0x000F1 };
static const wchar_t ntlgSemicolonEntityValue[] = { 0x02278 };
static const wchar_t ntriangleleftSemicolonEntityValue[] = { 0x022EA };
static const wchar_t ntrianglelefteqSemicolonEntityValue[] = { 0x022EC };
static const wchar_t ntrianglerightSemicolonEntityValue[] = { 0x022EB };
static const wchar_t ntrianglerighteqSemicolonEntityValue[] = { 0x022ED };
static const wchar_t nuSemicolonEntityValue[] = { 0x003BD };
static const wchar_t numSemicolonEntityValue[] = { 0x00023 };
static const wchar_t numeroSemicolonEntityValue[] = { 0x02116 };
static const wchar_t numspSemicolonEntityValue[] = { 0x02007 };
static const wchar_t nvDashSemicolonEntityValue[] = { 0x022AD };
static const wchar_t nvHarrSemicolonEntityValue[] = { 0x02904 };
static const wchar_t nvapSemicolonEntityValue[] = { 0x0224D, 0x020D2 };
static const wchar_t nvdashSemicolonEntityValue[] = { 0x022AC };
static const wchar_t nvgeSemicolonEntityValue[] = { 0x02265, 0x020D2 };
static const wchar_t nvgtSemicolonEntityValue[] = { 0x0003E, 0x020D2 };
static const wchar_t nvinfinSemicolonEntityValue[] = { 0x029DE };
static const wchar_t nvlArrSemicolonEntityValue[] = { 0x02902 };
static const wchar_t nvleSemicolonEntityValue[] = { 0x02264, 0x020D2 };
static const wchar_t nvltSemicolonEntityValue[] = { 0x0003C, 0x020D2 };
static const wchar_t nvltrieSemicolonEntityValue[] = { 0x022B4, 0x020D2 };
static const wchar_t nvrArrSemicolonEntityValue[] = { 0x02903 };
static const wchar_t nvrtrieSemicolonEntityValue[] = { 0x022B5, 0x020D2 };
static const wchar_t nvsimSemicolonEntityValue[] = { 0x0223C, 0x020D2 };
static const wchar_t nwArrSemicolonEntityValue[] = { 0x021D6 };
static const wchar_t nwarhkSemicolonEntityValue[] = { 0x02923 };
static const wchar_t nwarrSemicolonEntityValue[] = { 0x02196 };
static const wchar_t nwarrowSemicolonEntityValue[] = { 0x02196 };
static const wchar_t nwnearSemicolonEntityValue[] = { 0x02927 };
static const wchar_t oSSemicolonEntityValue[] = { 0x024C8 };
static const wchar_t oacuteEntityValue[] = { 0x000F3 };
static const wchar_t oacuteSemicolonEntityValue[] = { 0x000F3 };
static const wchar_t oastSemicolonEntityValue[] = { 0x0229B };
static const wchar_t ocirSemicolonEntityValue[] = { 0x0229A };
static const wchar_t ocircEntityValue[] = { 0x000F4 };
static const wchar_t ocircSemicolonEntityValue[] = { 0x000F4 };
static const wchar_t ocySemicolonEntityValue[] = { 0x0043E };
static const wchar_t odashSemicolonEntityValue[] = { 0x0229D };
static const wchar_t odblacSemicolonEntityValue[] = { 0x00151 };
static const wchar_t odivSemicolonEntityValue[] = { 0x02A38 };
static const wchar_t odotSemicolonEntityValue[] = { 0x02299 };
static const wchar_t odsoldSemicolonEntityValue[] = { 0x029BC };
static const wchar_t oeligSemicolonEntityValue[] = { 0x00153 };
static const wchar_t ofcirSemicolonEntityValue[] = { 0x029BF };
static const wchar_t ofrSemicolonEntityValue[] = { 0x1D52C };
static const wchar_t ogonSemicolonEntityValue[] = { 0x002DB };
static const wchar_t ograveEntityValue[] = { 0x000F2 };
static const wchar_t ograveSemicolonEntityValue[] = { 0x000F2 };
static const wchar_t ogtSemicolonEntityValue[] = { 0x029C1 };
static const wchar_t ohbarSemicolonEntityValue[] = { 0x029B5 };
static const wchar_t ohmSemicolonEntityValue[] = { 0x003A9 };
static const wchar_t ointSemicolonEntityValue[] = { 0x0222E };
static const wchar_t olarrSemicolonEntityValue[] = { 0x021BA };
static const wchar_t olcirSemicolonEntityValue[] = { 0x029BE };
static const wchar_t olcrossSemicolonEntityValue[] = { 0x029BB };
static const wchar_t olineSemicolonEntityValue[] = { 0x0203E };
static const wchar_t oltSemicolonEntityValue[] = { 0x029C0 };
static const wchar_t omacrSemicolonEntityValue[] = { 0x0014D };
static const wchar_t omegaSemicolonEntityValue[] = { 0x003C9 };
static const wchar_t omicronSemicolonEntityValue[] = { 0x003BF };
static const wchar_t omidSemicolonEntityValue[] = { 0x029B6 };
static const wchar_t ominusSemicolonEntityValue[] = { 0x02296 };
static const wchar_t oopfSemicolonEntityValue[] = { 0x1D560 };
static const wchar_t oparSemicolonEntityValue[] = { 0x029B7 };
static const wchar_t operpSemicolonEntityValue[] = { 0x029B9 };
static const wchar_t oplusSemicolonEntityValue[] = { 0x02295 };
static const wchar_t orSemicolonEntityValue[] = { 0x02228 };
static const wchar_t orarrSemicolonEntityValue[] = { 0x021BB };
static const wchar_t ordSemicolonEntityValue[] = { 0x02A5D };
static const wchar_t orderSemicolonEntityValue[] = { 0x02134 };
static const wchar_t orderofSemicolonEntityValue[] = { 0x02134 };
static const wchar_t ordfEntityValue[] = { 0x000AA };
static const wchar_t ordfSemicolonEntityValue[] = { 0x000AA };
static const wchar_t ordmEntityValue[] = { 0x000BA };
static const wchar_t ordmSemicolonEntityValue[] = { 0x000BA };
static const wchar_t origofSemicolonEntityValue[] = { 0x022B6 };
static const wchar_t ororSemicolonEntityValue[] = { 0x02A56 };
static const wchar_t orslopeSemicolonEntityValue[] = { 0x02A57 };
static const wchar_t orvSemicolonEntityValue[] = { 0x02A5B };
static const wchar_t oscrSemicolonEntityValue[] = { 0x02134 };
static const wchar_t oslashEntityValue[] = { 0x000F8 };
static const wchar_t oslashSemicolonEntityValue[] = { 0x000F8 };
static const wchar_t osolSemicolonEntityValue[] = { 0x02298 };
static const wchar_t otildeEntityValue[] = { 0x000F5 };
static const wchar_t otildeSemicolonEntityValue[] = { 0x000F5 };
static const wchar_t otimesSemicolonEntityValue[] = { 0x02297 };
static const wchar_t otimesasSemicolonEntityValue[] = { 0x02A36 };
static const wchar_t oumlEntityValue[] = { 0x000F6 };
static const wchar_t oumlSemicolonEntityValue[] = { 0x000F6 };
static const wchar_t ovbarSemicolonEntityValue[] = { 0x0233D };
static const wchar_t parSemicolonEntityValue[] = { 0x02225 };
static const wchar_t paraEntityValue[] = { 0x000B6 };
static const wchar_t paraSemicolonEntityValue[] = { 0x000B6 };
static const wchar_t parallelSemicolonEntityValue[] = { 0x02225 };
static const wchar_t parsimSemicolonEntityValue[] = { 0x02AF3 };
static const wchar_t parslSemicolonEntityValue[] = { 0x02AFD };
static const wchar_t partSemicolonEntityValue[] = { 0x02202 };
static const wchar_t pcySemicolonEntityValue[] = { 0x0043F };
static const wchar_t percntSemicolonEntityValue[] = { 0x00025 };
static const wchar_t periodSemicolonEntityValue[] = { 0x0002E };
static const wchar_t permilSemicolonEntityValue[] = { 0x02030 };
static const wchar_t perpSemicolonEntityValue[] = { 0x022A5 };
static const wchar_t pertenkSemicolonEntityValue[] = { 0x02031 };
static const wchar_t pfrSemicolonEntityValue[] = { 0x1D52D };
static const wchar_t phiSemicolonEntityValue[] = { 0x003C6 };
static const wchar_t phivSemicolonEntityValue[] = { 0x003D5 };
static const wchar_t phmmatSemicolonEntityValue[] = { 0x02133 };
static const wchar_t phoneSemicolonEntityValue[] = { 0x0260E };
static const wchar_t piSemicolonEntityValue[] = { 0x003C0 };
static const wchar_t pitchforkSemicolonEntityValue[] = { 0x022D4 };
static const wchar_t pivSemicolonEntityValue[] = { 0x003D6 };
static const wchar_t planckSemicolonEntityValue[] = { 0x0210F };
static const wchar_t planckhSemicolonEntityValue[] = { 0x0210E };
static const wchar_t plankvSemicolonEntityValue[] = { 0x0210F };
static const wchar_t plusSemicolonEntityValue[] = { 0x0002B };
static const wchar_t plusacirSemicolonEntityValue[] = { 0x02A23 };
static const wchar_t plusbSemicolonEntityValue[] = { 0x0229E };
static const wchar_t pluscirSemicolonEntityValue[] = { 0x02A22 };
static const wchar_t plusdoSemicolonEntityValue[] = { 0x02214 };
static const wchar_t plusduSemicolonEntityValue[] = { 0x02A25 };
static const wchar_t pluseSemicolonEntityValue[] = { 0x02A72 };
static const wchar_t plusmnEntityValue[] = { 0x000B1 };
static const wchar_t plusmnSemicolonEntityValue[] = { 0x000B1 };
static const wchar_t plussimSemicolonEntityValue[] = { 0x02A26 };
static const wchar_t plustwoSemicolonEntityValue[] = { 0x02A27 };
static const wchar_t pmSemicolonEntityValue[] = { 0x000B1 };
static const wchar_t pointintSemicolonEntityValue[] = { 0x02A15 };
static const wchar_t popfSemicolonEntityValue[] = { 0x1D561 };
static const wchar_t poundEntityValue[] = { 0x000A3 };
static const wchar_t poundSemicolonEntityValue[] = { 0x000A3 };
static const wchar_t prSemicolonEntityValue[] = { 0x0227A };
static const wchar_t prESemicolonEntityValue[] = { 0x02AB3 };
static const wchar_t prapSemicolonEntityValue[] = { 0x02AB7 };
static const wchar_t prcueSemicolonEntityValue[] = { 0x0227C };
static const wchar_t preSemicolonEntityValue[] = { 0x02AAF };
static const wchar_t precSemicolonEntityValue[] = { 0x0227A };
static const wchar_t precapproxSemicolonEntityValue[] = { 0x02AB7 };
static const wchar_t preccurlyeqSemicolonEntityValue[] = { 0x0227C };
static const wchar_t preceqSemicolonEntityValue[] = { 0x02AAF };
static const wchar_t precnapproxSemicolonEntityValue[] = { 0x02AB9 };
static const wchar_t precneqqSemicolonEntityValue[] = { 0x02AB5 };
static const wchar_t precnsimSemicolonEntityValue[] = { 0x022E8 };
static const wchar_t precsimSemicolonEntityValue[] = { 0x0227E };
static const wchar_t primeSemicolonEntityValue[] = { 0x02032 };
static const wchar_t primesSemicolonEntityValue[] = { 0x02119 };
static const wchar_t prnESemicolonEntityValue[] = { 0x02AB5 };
static const wchar_t prnapSemicolonEntityValue[] = { 0x02AB9 };
static const wchar_t prnsimSemicolonEntityValue[] = { 0x022E8 };
static const wchar_t prodSemicolonEntityValue[] = { 0x0220F };
static const wchar_t profalarSemicolonEntityValue[] = { 0x0232E };
static const wchar_t proflineSemicolonEntityValue[] = { 0x02312 };
static const wchar_t profsurfSemicolonEntityValue[] = { 0x02313 };
static const wchar_t propSemicolonEntityValue[] = { 0x0221D };
static const wchar_t proptoSemicolonEntityValue[] = { 0x0221D };
static const wchar_t prsimSemicolonEntityValue[] = { 0x0227E };
static const wchar_t prurelSemicolonEntityValue[] = { 0x022B0 };
static const wchar_t pscrSemicolonEntityValue[] = { 0x1D4C5 };
static const wchar_t psiSemicolonEntityValue[] = { 0x003C8 };
static const wchar_t puncspSemicolonEntityValue[] = { 0x02008 };
static const wchar_t qfrSemicolonEntityValue[] = { 0x1D52E };
static const wchar_t qintSemicolonEntityValue[] = { 0x02A0C };
static const wchar_t qopfSemicolonEntityValue[] = { 0x1D562 };
static const wchar_t qprimeSemicolonEntityValue[] = { 0x02057 };
static const wchar_t qscrSemicolonEntityValue[] = { 0x1D4C6 };
static const wchar_t quaternionsSemicolonEntityValue[] = { 0x0210D };
static const wchar_t quatintSemicolonEntityValue[] = { 0x02A16 };
static const wchar_t questSemicolonEntityValue[] = { 0x0003F };
static const wchar_t questeqSemicolonEntityValue[] = { 0x0225F };
static const wchar_t quotEntityValue[] = { 0x00022 };
static const wchar_t quotSemicolonEntityValue[] = { 0x00022 };
static const wchar_t rAarrSemicolonEntityValue[] = { 0x021DB };
static const wchar_t rArrSemicolonEntityValue[] = { 0x021D2 };
static const wchar_t rAtailSemicolonEntityValue[] = { 0x0291C };
static const wchar_t rBarrSemicolonEntityValue[] = { 0x0290F };
static const wchar_t rHarSemicolonEntityValue[] = { 0x02964 };
static const wchar_t raceSemicolonEntityValue[] = { 0x0223D, 0x00331 };
static const wchar_t racuteSemicolonEntityValue[] = { 0x00155 };
static const wchar_t radicSemicolonEntityValue[] = { 0x0221A };
static const wchar_t raemptyvSemicolonEntityValue[] = { 0x029B3 };
static const wchar_t rangSemicolonEntityValue[] = { 0x027E9 };
static const wchar_t rangdSemicolonEntityValue[] = { 0x02992 };
static const wchar_t rangeSemicolonEntityValue[] = { 0x029A5 };
static const wchar_t rangleSemicolonEntityValue[] = { 0x027E9 };
static const wchar_t raquoEntityValue[] = { 0x000BB };
static const wchar_t raquoSemicolonEntityValue[] = { 0x000BB };
static const wchar_t rarrSemicolonEntityValue[] = { 0x02192 };
static const wchar_t rarrapSemicolonEntityValue[] = { 0x02975 };
static const wchar_t rarrbSemicolonEntityValue[] = { 0x021E5 };
static const wchar_t rarrbfsSemicolonEntityValue[] = { 0x02920 };
static const wchar_t rarrcSemicolonEntityValue[] = { 0x02933 };
static const wchar_t rarrfsSemicolonEntityValue[] = { 0x0291E };
static const wchar_t rarrhkSemicolonEntityValue[] = { 0x021AA };
static const wchar_t rarrlpSemicolonEntityValue[] = { 0x021AC };
static const wchar_t rarrplSemicolonEntityValue[] = { 0x02945 };
static const wchar_t rarrsimSemicolonEntityValue[] = { 0x02974 };
static const wchar_t rarrtlSemicolonEntityValue[] = { 0x021A3 };
static const wchar_t rarrwSemicolonEntityValue[] = { 0x0219D };
static const wchar_t ratailSemicolonEntityValue[] = { 0x0291A };
static const wchar_t ratioSemicolonEntityValue[] = { 0x02236 };
static const wchar_t rationalsSemicolonEntityValue[] = { 0x0211A };
static const wchar_t rbarrSemicolonEntityValue[] = { 0x0290D };
static const wchar_t rbbrkSemicolonEntityValue[] = { 0x02773 };
static const wchar_t rbraceSemicolonEntityValue[] = { 0x0007D };
static const wchar_t rbrackSemicolonEntityValue[] = { 0x0005D };
static const wchar_t rbrkeSemicolonEntityValue[] = { 0x0298C };
static const wchar_t rbrksldSemicolonEntityValue[] = { 0x0298E };
static const wchar_t rbrksluSemicolonEntityValue[] = { 0x02990 };
static const wchar_t rcaronSemicolonEntityValue[] = { 0x00159 };
static const wchar_t rcedilSemicolonEntityValue[] = { 0x00157 };
static const wchar_t rceilSemicolonEntityValue[] = { 0x02309 };
static const wchar_t rcubSemicolonEntityValue[] = { 0x0007D };
static const wchar_t rcySemicolonEntityValue[] = { 0x00440 };
static const wchar_t rdcaSemicolonEntityValue[] = { 0x02937 };
static const wchar_t rdldharSemicolonEntityValue[] = { 0x02969 };
static const wchar_t rdquoSemicolonEntityValue[] = { 0x0201D };
static const wchar_t rdquorSemicolonEntityValue[] = { 0x0201D };
static const wchar_t rdshSemicolonEntityValue[] = { 0x021B3 };
static const wchar_t realSemicolonEntityValue[] = { 0x0211C };
static const wchar_t realineSemicolonEntityValue[] = { 0x0211B };
static const wchar_t realpartSemicolonEntityValue[] = { 0x0211C };
static const wchar_t realsSemicolonEntityValue[] = { 0x0211D };
static const wchar_t rectSemicolonEntityValue[] = { 0x025AD };
static const wchar_t regEntityValue[] = { 0x000AE };
static const wchar_t regSemicolonEntityValue[] = { 0x000AE };
static const wchar_t rfishtSemicolonEntityValue[] = { 0x0297D };
static const wchar_t rfloorSemicolonEntityValue[] = { 0x0230B };
static const wchar_t rfrSemicolonEntityValue[] = { 0x1D52F };
static const wchar_t rhardSemicolonEntityValue[] = { 0x021C1 };
static const wchar_t rharuSemicolonEntityValue[] = { 0x021C0 };
static const wchar_t rharulSemicolonEntityValue[] = { 0x0296C };
static const wchar_t rhoSemicolonEntityValue[] = { 0x003C1 };
static const wchar_t rhovSemicolonEntityValue[] = { 0x003F1 };
static const wchar_t rightarrowSemicolonEntityValue[] = { 0x02192 };
static const wchar_t rightarrowtailSemicolonEntityValue[] = { 0x021A3 };
static const wchar_t rightharpoondownSemicolonEntityValue[] = { 0x021C1 };
static const wchar_t rightharpoonupSemicolonEntityValue[] = { 0x021C0 };
static const wchar_t rightleftarrowsSemicolonEntityValue[] = { 0x021C4 };
static const wchar_t rightleftharpoonsSemicolonEntityValue[] = { 0x021CC };
static const wchar_t rightrightarrowsSemicolonEntityValue[] = { 0x021C9 };
static const wchar_t rightsquigarrowSemicolonEntityValue[] = { 0x0219D };
static const wchar_t rightthreetimesSemicolonEntityValue[] = { 0x022CC };
static const wchar_t ringSemicolonEntityValue[] = { 0x002DA };
static const wchar_t risingdotseqSemicolonEntityValue[] = { 0x02253 };
static const wchar_t rlarrSemicolonEntityValue[] = { 0x021C4 };
static const wchar_t rlharSemicolonEntityValue[] = { 0x021CC };
static const wchar_t rlmSemicolonEntityValue[] = { 0x0200F };
static const wchar_t rmoustSemicolonEntityValue[] = { 0x023B1 };
static const wchar_t rmoustacheSemicolonEntityValue[] = { 0x023B1 };
static const wchar_t rnmidSemicolonEntityValue[] = { 0x02AEE };
static const wchar_t roangSemicolonEntityValue[] = { 0x027ED };
static const wchar_t roarrSemicolonEntityValue[] = { 0x021FE };
static const wchar_t robrkSemicolonEntityValue[] = { 0x027E7 };
static const wchar_t roparSemicolonEntityValue[] = { 0x02986 };
static const wchar_t ropfSemicolonEntityValue[] = { 0x1D563 };
static const wchar_t roplusSemicolonEntityValue[] = { 0x02A2E };
static const wchar_t rotimesSemicolonEntityValue[] = { 0x02A35 };
static const wchar_t rparSemicolonEntityValue[] = { 0x00029 };
static const wchar_t rpargtSemicolonEntityValue[] = { 0x02994 };
static const wchar_t rppolintSemicolonEntityValue[] = { 0x02A12 };
static const wchar_t rrarrSemicolonEntityValue[] = { 0x021C9 };
static const wchar_t rsaquoSemicolonEntityValue[] = { 0x0203A };
static const wchar_t rscrSemicolonEntityValue[] = { 0x1D4C7 };
static const wchar_t rshSemicolonEntityValue[] = { 0x021B1 };
static const wchar_t rsqbSemicolonEntityValue[] = { 0x0005D };
static const wchar_t rsquoSemicolonEntityValue[] = { 0x02019 };
static const wchar_t rsquorSemicolonEntityValue[] = { 0x02019 };
static const wchar_t rthreeSemicolonEntityValue[] = { 0x022CC };
static const wchar_t rtimesSemicolonEntityValue[] = { 0x022CA };
static const wchar_t rtriSemicolonEntityValue[] = { 0x025B9 };
static const wchar_t rtrieSemicolonEntityValue[] = { 0x022B5 };
static const wchar_t rtrifSemicolonEntityValue[] = { 0x025B8 };
static const wchar_t rtriltriSemicolonEntityValue[] = { 0x029CE };
static const wchar_t ruluharSemicolonEntityValue[] = { 0x02968 };
static const wchar_t rxSemicolonEntityValue[] = { 0x0211E };
static const wchar_t sacuteSemicolonEntityValue[] = { 0x0015B };
static const wchar_t sbquoSemicolonEntityValue[] = { 0x0201A };
static const wchar_t scSemicolonEntityValue[] = { 0x0227B };
static const wchar_t scESemicolonEntityValue[] = { 0x02AB4 };
static const wchar_t scapSemicolonEntityValue[] = { 0x02AB8 };
static const wchar_t scaronSemicolonEntityValue[] = { 0x00161 };
static const wchar_t sccueSemicolonEntityValue[] = { 0x0227D };
static const wchar_t sceSemicolonEntityValue[] = { 0x02AB0 };
static const wchar_t scedilSemicolonEntityValue[] = { 0x0015F };
static const wchar_t scircSemicolonEntityValue[] = { 0x0015D };
static const wchar_t scnESemicolonEntityValue[] = { 0x02AB6 };
static const wchar_t scnapSemicolonEntityValue[] = { 0x02ABA };
static const wchar_t scnsimSemicolonEntityValue[] = { 0x022E9 };
static const wchar_t scpolintSemicolonEntityValue[] = { 0x02A13 };
static const wchar_t scsimSemicolonEntityValue[] = { 0x0227F };
static const wchar_t scySemicolonEntityValue[] = { 0x00441 };
static const wchar_t sdotSemicolonEntityValue[] = { 0x022C5 };
static const wchar_t sdotbSemicolonEntityValue[] = { 0x022A1 };
static const wchar_t sdoteSemicolonEntityValue[] = { 0x02A66 };
static const wchar_t seArrSemicolonEntityValue[] = { 0x021D8 };
static const wchar_t searhkSemicolonEntityValue[] = { 0x02925 };
static const wchar_t searrSemicolonEntityValue[] = { 0x02198 };
static const wchar_t searrowSemicolonEntityValue[] = { 0x02198 };
static const wchar_t sectEntityValue[] = { 0x000A7 };
static const wchar_t sectSemicolonEntityValue[] = { 0x000A7 };
static const wchar_t semiSemicolonEntityValue[] = { 0x0003B };
static const wchar_t seswarSemicolonEntityValue[] = { 0x02929 };
static const wchar_t setminusSemicolonEntityValue[] = { 0x02216 };
static const wchar_t setmnSemicolonEntityValue[] = { 0x02216 };
static const wchar_t sextSemicolonEntityValue[] = { 0x02736 };
static const wchar_t sfrSemicolonEntityValue[] = { 0x1D530 };
static const wchar_t sfrownSemicolonEntityValue[] = { 0x02322 };
static const wchar_t sharpSemicolonEntityValue[] = { 0x0266F };
static const wchar_t shchcySemicolonEntityValue[] = { 0x00449 };
static const wchar_t shcySemicolonEntityValue[] = { 0x00448 };
static const wchar_t shortmidSemicolonEntityValue[] = { 0x02223 };
static const wchar_t shortparallelSemicolonEntityValue[] = { 0x02225 };
static const wchar_t shyEntityValue[] = { 0x000AD };
static const wchar_t shySemicolonEntityValue[] = { 0x000AD };
static const wchar_t sigmaSemicolonEntityValue[] = { 0x003C3 };
static const wchar_t sigmafSemicolonEntityValue[] = { 0x003C2 };
static const wchar_t sigmavSemicolonEntityValue[] = { 0x003C2 };
static const wchar_t simSemicolonEntityValue[] = { 0x0223C };
static const wchar_t simdotSemicolonEntityValue[] = { 0x02A6A };
static const wchar_t simeSemicolonEntityValue[] = { 0x02243 };
static const wchar_t simeqSemicolonEntityValue[] = { 0x02243 };
static const wchar_t simgSemicolonEntityValue[] = { 0x02A9E };
static const wchar_t simgESemicolonEntityValue[] = { 0x02AA0 };
static const wchar_t simlSemicolonEntityValue[] = { 0x02A9D };
static const wchar_t simlESemicolonEntityValue[] = { 0x02A9F };
static const wchar_t simneSemicolonEntityValue[] = { 0x02246 };
static const wchar_t simplusSemicolonEntityValue[] = { 0x02A24 };
static const wchar_t simrarrSemicolonEntityValue[] = { 0x02972 };
static const wchar_t slarrSemicolonEntityValue[] = { 0x02190 };
static const wchar_t smallsetminusSemicolonEntityValue[] = { 0x02216 };
static const wchar_t smashpSemicolonEntityValue[] = { 0x02A33 };
static const wchar_t smeparslSemicolonEntityValue[] = { 0x029E4 };
static const wchar_t smidSemicolonEntityValue[] = { 0x02223 };
static const wchar_t smileSemicolonEntityValue[] = { 0x02323 };
static const wchar_t smtSemicolonEntityValue[] = { 0x02AAA };
static const wchar_t smteSemicolonEntityValue[] = { 0x02AAC };
static const wchar_t smtesSemicolonEntityValue[] = { 0x02AAC, 0x0FE00 };
static const wchar_t softcySemicolonEntityValue[] = { 0x0044C };
static const wchar_t solSemicolonEntityValue[] = { 0x0002F };
static const wchar_t solbSemicolonEntityValue[] = { 0x029C4 };
static const wchar_t solbarSemicolonEntityValue[] = { 0x0233F };
static const wchar_t sopfSemicolonEntityValue[] = { 0x1D564 };
static const wchar_t spadesSemicolonEntityValue[] = { 0x02660 };
static const wchar_t spadesuitSemicolonEntityValue[] = { 0x02660 };
static const wchar_t sparSemicolonEntityValue[] = { 0x02225 };
static const wchar_t sqcapSemicolonEntityValue[] = { 0x02293 };
static const wchar_t sqcapsSemicolonEntityValue[] = { 0x02293, 0x0FE00 };
static const wchar_t sqcupSemicolonEntityValue[] = { 0x02294 };
static const wchar_t sqcupsSemicolonEntityValue[] = { 0x02294, 0x0FE00 };
static const wchar_t sqsubSemicolonEntityValue[] = { 0x0228F };
static const wchar_t sqsubeSemicolonEntityValue[] = { 0x02291 };
static const wchar_t sqsubsetSemicolonEntityValue[] = { 0x0228F };
static const wchar_t sqsubseteqSemicolonEntityValue[] = { 0x02291 };
static const wchar_t sqsupSemicolonEntityValue[] = { 0x02290 };
static const wchar_t sqsupeSemicolonEntityValue[] = { 0x02292 };
static const wchar_t sqsupsetSemicolonEntityValue[] = { 0x02290 };
static const wchar_t sqsupseteqSemicolonEntityValue[] = { 0x02292 };
static const wchar_t squSemicolonEntityValue[] = { 0x025A1 };
static const wchar_t squareSemicolonEntityValue[] = { 0x025A1 };
static const wchar_t squarfSemicolonEntityValue[] = { 0x025AA };
static const wchar_t squfSemicolonEntityValue[] = { 0x025AA };
static const wchar_t srarrSemicolonEntityValue[] = { 0x02192 };
static const wchar_t sscrSemicolonEntityValue[] = { 0x1D4C8 };
static const wchar_t ssetmnSemicolonEntityValue[] = { 0x02216 };
static const wchar_t ssmileSemicolonEntityValue[] = { 0x02323 };
static const wchar_t sstarfSemicolonEntityValue[] = { 0x022C6 };
static const wchar_t starSemicolonEntityValue[] = { 0x02606 };
static const wchar_t starfSemicolonEntityValue[] = { 0x02605 };
static const wchar_t straightepsilonSemicolonEntityValue[] = { 0x003F5 };
static const wchar_t straightphiSemicolonEntityValue[] = { 0x003D5 };
static const wchar_t strnsSemicolonEntityValue[] = { 0x000AF };
static const wchar_t subSemicolonEntityValue[] = { 0x02282 };
static const wchar_t subESemicolonEntityValue[] = { 0x02AC5 };
static const wchar_t subdotSemicolonEntityValue[] = { 0x02ABD };
static const wchar_t subeSemicolonEntityValue[] = { 0x02286 };
static const wchar_t subedotSemicolonEntityValue[] = { 0x02AC3 };
static const wchar_t submultSemicolonEntityValue[] = { 0x02AC1 };
static const wchar_t subnESemicolonEntityValue[] = { 0x02ACB };
static const wchar_t subneSemicolonEntityValue[] = { 0x0228A };
static const wchar_t subplusSemicolonEntityValue[] = { 0x02ABF };
static const wchar_t subrarrSemicolonEntityValue[] = { 0x02979 };
static const wchar_t subsetSemicolonEntityValue[] = { 0x02282 };
static const wchar_t subseteqSemicolonEntityValue[] = { 0x02286 };
static const wchar_t subseteqqSemicolonEntityValue[] = { 0x02AC5 };
static const wchar_t subsetneqSemicolonEntityValue[] = { 0x0228A };
static const wchar_t subsetneqqSemicolonEntityValue[] = { 0x02ACB };
static const wchar_t subsimSemicolonEntityValue[] = { 0x02AC7 };
static const wchar_t subsubSemicolonEntityValue[] = { 0x02AD5 };
static const wchar_t subsupSemicolonEntityValue[] = { 0x02AD3 };
static const wchar_t succSemicolonEntityValue[] = { 0x0227B };
static const wchar_t succapproxSemicolonEntityValue[] = { 0x02AB8 };
static const wchar_t succcurlyeqSemicolonEntityValue[] = { 0x0227D };
static const wchar_t succeqSemicolonEntityValue[] = { 0x02AB0 };
static const wchar_t succnapproxSemicolonEntityValue[] = { 0x02ABA };
static const wchar_t succneqqSemicolonEntityValue[] = { 0x02AB6 };
static const wchar_t succnsimSemicolonEntityValue[] = { 0x022E9 };
static const wchar_t succsimSemicolonEntityValue[] = { 0x0227F };
static const wchar_t sumSemicolonEntityValue[] = { 0x02211 };
static const wchar_t sungSemicolonEntityValue[] = { 0x0266A };
static const wchar_t sup1EntityValue[] = { 0x000B9 };
static const wchar_t sup1SemicolonEntityValue[] = { 0x000B9 };
static const wchar_t sup2EntityValue[] = { 0x000B2 };
static const wchar_t sup2SemicolonEntityValue[] = { 0x000B2 };
static const wchar_t sup3EntityValue[] = { 0x000B3 };
static const wchar_t sup3SemicolonEntityValue[] = { 0x000B3 };
static const wchar_t supSemicolonEntityValue[] = { 0x02283 };
static const wchar_t supESemicolonEntityValue[] = { 0x02AC6 };
static const wchar_t supdotSemicolonEntityValue[] = { 0x02ABE };
static const wchar_t supdsubSemicolonEntityValue[] = { 0x02AD8 };
static const wchar_t supeSemicolonEntityValue[] = { 0x02287 };
static const wchar_t supedotSemicolonEntityValue[] = { 0x02AC4 };
static const wchar_t suphsolSemicolonEntityValue[] = { 0x027C9 };
static const wchar_t suphsubSemicolonEntityValue[] = { 0x02AD7 };
static const wchar_t suplarrSemicolonEntityValue[] = { 0x0297B };
static const wchar_t supmultSemicolonEntityValue[] = { 0x02AC2 };
static const wchar_t supnESemicolonEntityValue[] = { 0x02ACC };
static const wchar_t supneSemicolonEntityValue[] = { 0x0228B };
static const wchar_t supplusSemicolonEntityValue[] = { 0x02AC0 };
static const wchar_t supsetSemicolonEntityValue[] = { 0x02283 };
static const wchar_t supseteqSemicolonEntityValue[] = { 0x02287 };
static const wchar_t supseteqqSemicolonEntityValue[] = { 0x02AC6 };
static const wchar_t supsetneqSemicolonEntityValue[] = { 0x0228B };
static const wchar_t supsetneqqSemicolonEntityValue[] = { 0x02ACC };
static const wchar_t supsimSemicolonEntityValue[] = { 0x02AC8 };
static const wchar_t supsubSemicolonEntityValue[] = { 0x02AD4 };
static const wchar_t supsupSemicolonEntityValue[] = { 0x02AD6 };
static const wchar_t swArrSemicolonEntityValue[] = { 0x021D9 };
static const wchar_t swarhkSemicolonEntityValue[] = { 0x02926 };
static const wchar_t swarrSemicolonEntityValue[] = { 0x02199 };
static const wchar_t swarrowSemicolonEntityValue[] = { 0x02199 };
static const wchar_t swnwarSemicolonEntityValue[] = { 0x0292A };
static const wchar_t szligEntityValue[] = { 0x000DF };
static const wchar_t szligSemicolonEntityValue[] = { 0x000DF };
static const wchar_t targetSemicolonEntityValue[] = { 0x02316 };
static const wchar_t tauSemicolonEntityValue[] = { 0x003C4 };
static const wchar_t tbrkSemicolonEntityValue[] = { 0x023B4 };
static const wchar_t tcaronSemicolonEntityValue[] = { 0x00165 };
static const wchar_t tcedilSemicolonEntityValue[] = { 0x00163 };
static const wchar_t tcySemicolonEntityValue[] = { 0x00442 };
static const wchar_t tdotSemicolonEntityValue[] = { 0x020DB };
static const wchar_t telrecSemicolonEntityValue[] = { 0x02315 };
static const wchar_t tfrSemicolonEntityValue[] = { 0x1D531 };
static const wchar_t there4SemicolonEntityValue[] = { 0x02234 };
static const wchar_t thereforeSemicolonEntityValue[] = { 0x02234 };
static const wchar_t thetaSemicolonEntityValue[] = { 0x003B8 };
static const wchar_t thetasymSemicolonEntityValue[] = { 0x003D1 };
static const wchar_t thetavSemicolonEntityValue[] = { 0x003D1 };
static const wchar_t thickapproxSemicolonEntityValue[] = { 0x02248 };
static const wchar_t thicksimSemicolonEntityValue[] = { 0x0223C };
static const wchar_t thinspSemicolonEntityValue[] = { 0x02009 };
static const wchar_t thkapSemicolonEntityValue[] = { 0x02248 };
static const wchar_t thksimSemicolonEntityValue[] = { 0x0223C };
static const wchar_t thornEntityValue[] = { 0x000FE };
static const wchar_t thornSemicolonEntityValue[] = { 0x000FE };
static const wchar_t tildeSemicolonEntityValue[] = { 0x002DC };
static const wchar_t timesEntityValue[] = { 0x000D7 };
static const wchar_t timesSemicolonEntityValue[] = { 0x000D7 };
static const wchar_t timesbSemicolonEntityValue[] = { 0x022A0 };
static const wchar_t timesbarSemicolonEntityValue[] = { 0x02A31 };
static const wchar_t timesdSemicolonEntityValue[] = { 0x02A30 };
static const wchar_t tintSemicolonEntityValue[] = { 0x0222D };
static const wchar_t toeaSemicolonEntityValue[] = { 0x02928 };
static const wchar_t topSemicolonEntityValue[] = { 0x022A4 };
static const wchar_t topbotSemicolonEntityValue[] = { 0x02336 };
static const wchar_t topcirSemicolonEntityValue[] = { 0x02AF1 };
static const wchar_t topfSemicolonEntityValue[] = { 0x1D565 };
static const wchar_t topforkSemicolonEntityValue[] = { 0x02ADA };
static const wchar_t tosaSemicolonEntityValue[] = { 0x02929 };
static const wchar_t tprimeSemicolonEntityValue[] = { 0x02034 };
static const wchar_t tradeSemicolonEntityValue[] = { 0x02122 };
static const wchar_t triangleSemicolonEntityValue[] = { 0x025B5 };
static const wchar_t triangledownSemicolonEntityValue[] = { 0x025BF };
static const wchar_t triangleleftSemicolonEntityValue[] = { 0x025C3 };
static const wchar_t trianglelefteqSemicolonEntityValue[] = { 0x022B4 };
static const wchar_t triangleqSemicolonEntityValue[] = { 0x0225C };
static const wchar_t trianglerightSemicolonEntityValue[] = { 0x025B9 };
static const wchar_t trianglerighteqSemicolonEntityValue[] = { 0x022B5 };
static const wchar_t tridotSemicolonEntityValue[] = { 0x025EC };
static const wchar_t trieSemicolonEntityValue[] = { 0x0225C };
static const wchar_t triminusSemicolonEntityValue[] = { 0x02A3A };
static const wchar_t triplusSemicolonEntityValue[] = { 0x02A39 };
static const wchar_t trisbSemicolonEntityValue[] = { 0x029CD };
static const wchar_t tritimeSemicolonEntityValue[] = { 0x02A3B };
static const wchar_t trpeziumSemicolonEntityValue[] = { 0x023E2 };
static const wchar_t tscrSemicolonEntityValue[] = { 0x1D4C9 };
static const wchar_t tscySemicolonEntityValue[] = { 0x00446 };
static const wchar_t tshcySemicolonEntityValue[] = { 0x0045B };
static const wchar_t tstrokSemicolonEntityValue[] = { 0x00167 };
static const wchar_t twixtSemicolonEntityValue[] = { 0x0226C };
static const wchar_t twoheadleftarrowSemicolonEntityValue[] = { 0x0219E };
static const wchar_t twoheadrightarrowSemicolonEntityValue[] = { 0x021A0 };
static const wchar_t uArrSemicolonEntityValue[] = { 0x021D1 };
static const wchar_t uHarSemicolonEntityValue[] = { 0x02963 };
static const wchar_t uacuteEntityValue[] = { 0x000FA };
static const wchar_t uacuteSemicolonEntityValue[] = { 0x000FA };
static const wchar_t uarrSemicolonEntityValue[] = { 0x02191 };
static const wchar_t ubrcySemicolonEntityValue[] = { 0x0045E };
static const wchar_t ubreveSemicolonEntityValue[] = { 0x0016D };
static const wchar_t ucircEntityValue[] = { 0x000FB };
static const wchar_t ucircSemicolonEntityValue[] = { 0x000FB };
static const wchar_t ucySemicolonEntityValue[] = { 0x00443 };
static const wchar_t udarrSemicolonEntityValue[] = { 0x021C5 };
static const wchar_t udblacSemicolonEntityValue[] = { 0x00171 };
static const wchar_t udharSemicolonEntityValue[] = { 0x0296E };
static const wchar_t ufishtSemicolonEntityValue[] = { 0x0297E };
static const wchar_t ufrSemicolonEntityValue[] = { 0x1D532 };
static const wchar_t ugraveEntityValue[] = { 0x000F9 };
static const wchar_t ugraveSemicolonEntityValue[] = { 0x000F9 };
static const wchar_t uharlSemicolonEntityValue[] = { 0x021BF };
static const wchar_t uharrSemicolonEntityValue[] = { 0x021BE };
static const wchar_t uhblkSemicolonEntityValue[] = { 0x02580 };
static const wchar_t ulcornSemicolonEntityValue[] = { 0x0231C };
static const wchar_t ulcornerSemicolonEntityValue[] = { 0x0231C };
static const wchar_t ulcropSemicolonEntityValue[] = { 0x0230F };
static const wchar_t ultriSemicolonEntityValue[] = { 0x025F8 };
static const wchar_t umacrSemicolonEntityValue[] = { 0x0016B };
static const wchar_t umlEntityValue[] = { 0x000A8 };
static const wchar_t umlSemicolonEntityValue[] = { 0x000A8 };
static const wchar_t uogonSemicolonEntityValue[] = { 0x00173 };
static const wchar_t uopfSemicolonEntityValue[] = { 0x1D566 };
static const wchar_t uparrowSemicolonEntityValue[] = { 0x02191 };
static const wchar_t updownarrowSemicolonEntityValue[] = { 0x02195 };
static const wchar_t upharpoonleftSemicolonEntityValue[] = { 0x021BF };
static const wchar_t upharpoonrightSemicolonEntityValue[] = { 0x021BE };
static const wchar_t uplusSemicolonEntityValue[] = { 0x0228E };
static const wchar_t upsiSemicolonEntityValue[] = { 0x003C5 };
static const wchar_t upsihSemicolonEntityValue[] = { 0x003D2 };
static const wchar_t upsilonSemicolonEntityValue[] = { 0x003C5 };
static const wchar_t upuparrowsSemicolonEntityValue[] = { 0x021C8 };
static const wchar_t urcornSemicolonEntityValue[] = { 0x0231D };
static const wchar_t urcornerSemicolonEntityValue[] = { 0x0231D };
static const wchar_t urcropSemicolonEntityValue[] = { 0x0230E };
static const wchar_t uringSemicolonEntityValue[] = { 0x0016F };
static const wchar_t urtriSemicolonEntityValue[] = { 0x025F9 };
static const wchar_t uscrSemicolonEntityValue[] = { 0x1D4CA };
static const wchar_t utdotSemicolonEntityValue[] = { 0x022F0 };
static const wchar_t utildeSemicolonEntityValue[] = { 0x00169 };
static const wchar_t utriSemicolonEntityValue[] = { 0x025B5 };
static const wchar_t utrifSemicolonEntityValue[] = { 0x025B4 };
static const wchar_t uuarrSemicolonEntityValue[] = { 0x021C8 };
static const wchar_t uumlEntityValue[] = { 0x000FC };
static const wchar_t uumlSemicolonEntityValue[] = { 0x000FC };
static const wchar_t uwangleSemicolonEntityValue[] = { 0x029A7 };
static const wchar_t vArrSemicolonEntityValue[] = { 0x021D5 };
static const wchar_t vBarSemicolonEntityValue[] = { 0x02AE8 };
static const wchar_t vBarvSemicolonEntityValue[] = { 0x02AE9 };
static const wchar_t vDashSemicolonEntityValue[] = { 0x022A8 };
static const wchar_t vangrtSemicolonEntityValue[] = { 0x0299C };
static const wchar_t varepsilonSemicolonEntityValue[] = { 0x003F5 };
static const wchar_t varkappaSemicolonEntityValue[] = { 0x003F0 };
static const wchar_t varnothingSemicolonEntityValue[] = { 0x02205 };
static const wchar_t varphiSemicolonEntityValue[] = { 0x003D5 };
static const wchar_t varpiSemicolonEntityValue[] = { 0x003D6 };
static const wchar_t varproptoSemicolonEntityValue[] = { 0x0221D };
static const wchar_t varrSemicolonEntityValue[] = { 0x02195 };
static const wchar_t varrhoSemicolonEntityValue[] = { 0x003F1 };
static const wchar_t varsigmaSemicolonEntityValue[] = { 0x003C2 };
static const wchar_t varsubsetneqSemicolonEntityValue[] = { 0x0228A, 0x0FE00 };
static const wchar_t varsubsetneqqSemicolonEntityValue[] = { 0x02ACB, 0x0FE00 };
static const wchar_t varsupsetneqSemicolonEntityValue[] = { 0x0228B, 0x0FE00 };
static const wchar_t varsupsetneqqSemicolonEntityValue[] = { 0x02ACC, 0x0FE00 };
static const wchar_t varthetaSemicolonEntityValue[] = { 0x003D1 };
static const wchar_t vartriangleleftSemicolonEntityValue[] = { 0x022B2 };
static const wchar_t vartrianglerightSemicolonEntityValue[] = { 0x022B3 };
static const wchar_t vcySemicolonEntityValue[] = { 0x00432 };
static const wchar_t vdashSemicolonEntityValue[] = { 0x022A2 };
static const wchar_t veeSemicolonEntityValue[] = { 0x02228 };
static const wchar_t veebarSemicolonEntityValue[] = { 0x022BB };
static const wchar_t veeeqSemicolonEntityValue[] = { 0x0225A };
static const wchar_t vellipSemicolonEntityValue[] = { 0x022EE };
static const wchar_t verbarSemicolonEntityValue[] = { 0x0007C };
static const wchar_t vertSemicolonEntityValue[] = { 0x0007C };
static const wchar_t vfrSemicolonEntityValue[] = { 0x1D533 };
static const wchar_t vltriSemicolonEntityValue[] = { 0x022B2 };
static const wchar_t vnsubSemicolonEntityValue[] = { 0x02282, 0x020D2 };
static const wchar_t vnsupSemicolonEntityValue[] = { 0x02283, 0x020D2 };
static const wchar_t vopfSemicolonEntityValue[] = { 0x1D567 };
static const wchar_t vpropSemicolonEntityValue[] = { 0x0221D };
static const wchar_t vrtriSemicolonEntityValue[] = { 0x022B3 };
static const wchar_t vscrSemicolonEntityValue[] = { 0x1D4CB };
static const wchar_t vsubnESemicolonEntityValue[] = { 0x02ACB, 0x0FE00 };
static const wchar_t vsubneSemicolonEntityValue[] = { 0x0228A, 0x0FE00 };
static const wchar_t vsupnESemicolonEntityValue[] = { 0x02ACC, 0x0FE00 };
static const wchar_t vsupneSemicolonEntityValue[] = { 0x0228B, 0x0FE00 };
static const wchar_t vzigzagSemicolonEntityValue[] = { 0x0299A };
static const wchar_t wcircSemicolonEntityValue[] = { 0x00175 };
static const wchar_t wedbarSemicolonEntityValue[] = { 0x02A5F };
static const wchar_t wedgeSemicolonEntityValue[] = { 0x02227 };
static const wchar_t wedgeqSemicolonEntityValue[] = { 0x02259 };
static const wchar_t weierpSemicolonEntityValue[] = { 0x02118 };
static const wchar_t wfrSemicolonEntityValue[] = { 0x1D534 };
static const wchar_t wopfSemicolonEntityValue[] = { 0x1D568 };
static const wchar_t wpSemicolonEntityValue[] = { 0x02118 };
static const wchar_t wrSemicolonEntityValue[] = { 0x02240 };
static const wchar_t wreathSemicolonEntityValue[] = { 0x02240 };
static const wchar_t wscrSemicolonEntityValue[] = { 0x1D4CC };
static const wchar_t xcapSemicolonEntityValue[] = { 0x022C2 };
static const wchar_t xcircSemicolonEntityValue[] = { 0x025EF };
static const wchar_t xcupSemicolonEntityValue[] = { 0x022C3 };
static const wchar_t xdtriSemicolonEntityValue[] = { 0x025BD };
static const wchar_t xfrSemicolonEntityValue[] = { 0x1D535 };
static const wchar_t xhArrSemicolonEntityValue[] = { 0x027FA };
static const wchar_t xharrSemicolonEntityValue[] = { 0x027F7 };
static const wchar_t xiSemicolonEntityValue[] = { 0x003BE };
static const wchar_t xlArrSemicolonEntityValue[] = { 0x027F8 };
static const wchar_t xlarrSemicolonEntityValue[] = { 0x027F5 };
static const wchar_t xmapSemicolonEntityValue[] = { 0x027FC };
static const wchar_t xnisSemicolonEntityValue[] = { 0x022FB };
static const wchar_t xodotSemicolonEntityValue[] = { 0x02A00 };
static const wchar_t xopfSemicolonEntityValue[] = { 0x1D569 };
static const wchar_t xoplusSemicolonEntityValue[] = { 0x02A01 };
static const wchar_t xotimeSemicolonEntityValue[] = { 0x02A02 };
static const wchar_t xrArrSemicolonEntityValue[] = { 0x027F9 };
static const wchar_t xrarrSemicolonEntityValue[] = { 0x027F6 };
static const wchar_t xscrSemicolonEntityValue[] = { 0x1D4CD };
static const wchar_t xsqcupSemicolonEntityValue[] = { 0x02A06 };
static const wchar_t xuplusSemicolonEntityValue[] = { 0x02A04 };
static const wchar_t xutriSemicolonEntityValue[] = { 0x025B3 };
static const wchar_t xveeSemicolonEntityValue[] = { 0x022C1 };
static const wchar_t xwedgeSemicolonEntityValue[] = { 0x022C0 };
static const wchar_t yacuteEntityValue[] = { 0x000FD };
static const wchar_t yacuteSemicolonEntityValue[] = { 0x000FD };
static const wchar_t yacySemicolonEntityValue[] = { 0x0044F };
static const wchar_t ycircSemicolonEntityValue[] = { 0x00177 };
static const wchar_t ycySemicolonEntityValue[] = { 0x0044B };
static const wchar_t yenEntityValue[] = { 0x000A5 };
static const wchar_t yenSemicolonEntityValue[] = { 0x000A5 };
static const wchar_t yfrSemicolonEntityValue[] = { 0x1D536 };
static const wchar_t yicySemicolonEntityValue[] = { 0x00457 };
static const wchar_t yopfSemicolonEntityValue[] = { 0x1D56A };
static const wchar_t yscrSemicolonEntityValue[] = { 0x1D4CE };
static const wchar_t yucySemicolonEntityValue[] = { 0x0044E };
static const wchar_t yumlEntityValue[] = { 0x000FF };
static const wchar_t yumlSemicolonEntityValue[] = { 0x000FF };
static const wchar_t zacuteSemicolonEntityValue[] = { 0x0017A };
static const wchar_t zcaronSemicolonEntityValue[] = { 0x0017E };
static const wchar_t zcySemicolonEntityValue[] = { 0x00437 };
static const wchar_t zdotSemicolonEntityValue[] = { 0x0017C };
static const wchar_t zeetrfSemicolonEntityValue[] = { 0x02128 };
static const wchar_t zetaSemicolonEntityValue[] = { 0x003B6 };
static const wchar_t zfrSemicolonEntityValue[] = { 0x1D537 };
static const wchar_t zhcySemicolonEntityValue[] = { 0x00436 };
static const wchar_t zigrarrSemicolonEntityValue[] = { 0x021DD };
static const wchar_t zopfSemicolonEntityValue[] = { 0x1D56B };
static const wchar_t zscrSemicolonEntityValue[] = { 0x1D4CF };
static const wchar_t zwjSemicolonEntityValue[] = { 0x0200D };
static const wchar_t zwnjSemicolonEntityValue[] = { 0x0200C };

static const
struct pchvml_entity pchvml_character_reference_entity_table[2231] = {
    { AEligEntityName, 5, AEligEntityValue, 1 },
    { AEligSemicolonEntityName, 6, AEligSemicolonEntityValue, 1 },
    { AMPEntityName, 3, AMPEntityValue, 1 },
    { AMPSemicolonEntityName, 4, AMPSemicolonEntityValue, 1 },
    { AacuteEntityName, 6, AacuteEntityValue, 1 },
    { AacuteSemicolonEntityName, 7, AacuteSemicolonEntityValue, 1 },
    { AbreveSemicolonEntityName, 7, AbreveSemicolonEntityValue, 1 },
    { AcircEntityName, 5, AcircEntityValue, 1 },
    { AcircSemicolonEntityName, 6, AcircSemicolonEntityValue, 1 },
    { AcySemicolonEntityName, 4, AcySemicolonEntityValue, 1 },
    { AfrSemicolonEntityName, 4, AfrSemicolonEntityValue, 1 },
    { AgraveEntityName, 6, AgraveEntityValue, 1 },
    { AgraveSemicolonEntityName, 7, AgraveSemicolonEntityValue, 1 },
    { AlphaSemicolonEntityName, 6, AlphaSemicolonEntityValue, 1 },
    { AmacrSemicolonEntityName, 6, AmacrSemicolonEntityValue, 1 },
    { AndSemicolonEntityName, 4, AndSemicolonEntityValue, 1 },
    { AogonSemicolonEntityName, 6, AogonSemicolonEntityValue, 1 },
    { AopfSemicolonEntityName, 5, AopfSemicolonEntityValue, 1 },
    { ApplyFunctionSemicolonEntityName, 14, ApplyFunctionSemicolonEntityValue, 1 },
    { AringEntityName, 5, AringEntityValue, 1 },
    { AringSemicolonEntityName, 6, AringSemicolonEntityValue, 1 },
    { AscrSemicolonEntityName, 5, AscrSemicolonEntityValue, 1 },
    { AssignSemicolonEntityName, 7, AssignSemicolonEntityValue, 1 },
    { AtildeEntityName, 6, AtildeEntityValue, 1 },
    { AtildeSemicolonEntityName, 7, AtildeSemicolonEntityValue, 1 },
    { AumlEntityName, 4, AumlEntityValue, 1 },
    { AumlSemicolonEntityName, 5, AumlSemicolonEntityValue, 1 },
    { BackslashSemicolonEntityName, 10, BackslashSemicolonEntityValue, 1 },
    { BarvSemicolonEntityName, 5, BarvSemicolonEntityValue, 1 },
    { BarwedSemicolonEntityName, 7, BarwedSemicolonEntityValue, 1 },
    { BcySemicolonEntityName, 4, BcySemicolonEntityValue, 1 },
    { BecauseSemicolonEntityName, 8, BecauseSemicolonEntityValue, 1 },
    { BernoullisSemicolonEntityName, 11, BernoullisSemicolonEntityValue, 1 },
    { BetaSemicolonEntityName, 5, BetaSemicolonEntityValue, 1 },
    { BfrSemicolonEntityName, 4, BfrSemicolonEntityValue, 1 },
    { BopfSemicolonEntityName, 5, BopfSemicolonEntityValue, 1 },
    { BreveSemicolonEntityName, 6, BreveSemicolonEntityValue, 1 },
    { BscrSemicolonEntityName, 5, BscrSemicolonEntityValue, 1 },
    { BumpeqSemicolonEntityName, 7, BumpeqSemicolonEntityValue, 1 },
    { CHcySemicolonEntityName, 5, CHcySemicolonEntityValue, 1 },
    { COPYEntityName, 4, COPYEntityValue, 1 },
    { COPYSemicolonEntityName, 5, COPYSemicolonEntityValue, 1 },
    { CacuteSemicolonEntityName, 7, CacuteSemicolonEntityValue, 1 },
    { CapSemicolonEntityName, 4, CapSemicolonEntityValue, 1 },
    { CapitalDifferentialDSemicolonEntityName, 21, CapitalDifferentialDSemicolonEntityValue, 1 },
    { CayleysSemicolonEntityName, 8, CayleysSemicolonEntityValue, 1 },
    { CcaronSemicolonEntityName, 7, CcaronSemicolonEntityValue, 1 },
    { CcedilEntityName, 6, CcedilEntityValue, 1 },
    { CcedilSemicolonEntityName, 7, CcedilSemicolonEntityValue, 1 },
    { CcircSemicolonEntityName, 6, CcircSemicolonEntityValue, 1 },
    { CconintSemicolonEntityName, 8, CconintSemicolonEntityValue, 1 },
    { CdotSemicolonEntityName, 5, CdotSemicolonEntityValue, 1 },
    { CedillaSemicolonEntityName, 8, CedillaSemicolonEntityValue, 1 },
    { CenterDotSemicolonEntityName, 10, CenterDotSemicolonEntityValue, 1 },
    { CfrSemicolonEntityName, 4, CfrSemicolonEntityValue, 1 },
    { ChiSemicolonEntityName, 4, ChiSemicolonEntityValue, 1 },
    { CircleDotSemicolonEntityName, 10, CircleDotSemicolonEntityValue, 1 },
    { CircleMinusSemicolonEntityName, 12, CircleMinusSemicolonEntityValue, 1 },
    { CirclePlusSemicolonEntityName, 11, CirclePlusSemicolonEntityValue, 1 },
    { CircleTimesSemicolonEntityName, 12, CircleTimesSemicolonEntityValue, 1 },
    { ClockwiseContourIntegralSemicolonEntityName, 25, ClockwiseContourIntegralSemicolonEntityValue, 1 },
    { CloseCurlyDoubleQuoteSemicolonEntityName, 22, CloseCurlyDoubleQuoteSemicolonEntityValue, 1 },
    { CloseCurlyQuoteSemicolonEntityName, 16, CloseCurlyQuoteSemicolonEntityValue, 1 },
    { ColonSemicolonEntityName, 6, ColonSemicolonEntityValue, 1 },
    { ColoneSemicolonEntityName, 7, ColoneSemicolonEntityValue, 1 },
    { CongruentSemicolonEntityName, 10, CongruentSemicolonEntityValue, 1 },
    { ConintSemicolonEntityName, 7, ConintSemicolonEntityValue, 1 },
    { ContourIntegralSemicolonEntityName, 16, ContourIntegralSemicolonEntityValue, 1 },
    { CopfSemicolonEntityName, 5, CopfSemicolonEntityValue, 1 },
    { CoproductSemicolonEntityName, 10, CoproductSemicolonEntityValue, 1 },
    { CounterClockwiseContourIntegralSemicolonEntityName, 32, CounterClockwiseContourIntegralSemicolonEntityValue, 1 },
    { CrossSemicolonEntityName, 6, CrossSemicolonEntityValue, 1 },
    { CscrSemicolonEntityName, 5, CscrSemicolonEntityValue, 1 },
    { CupSemicolonEntityName, 4, CupSemicolonEntityValue, 1 },
    { CupCapSemicolonEntityName, 7, CupCapSemicolonEntityValue, 1 },
    { DDSemicolonEntityName, 3, DDSemicolonEntityValue, 1 },
    { DDotrahdSemicolonEntityName, 9, DDotrahdSemicolonEntityValue, 1 },
    { DJcySemicolonEntityName, 5, DJcySemicolonEntityValue, 1 },
    { DScySemicolonEntityName, 5, DScySemicolonEntityValue, 1 },
    { DZcySemicolonEntityName, 5, DZcySemicolonEntityValue, 1 },
    { DaggerSemicolonEntityName, 7, DaggerSemicolonEntityValue, 1 },
    { DarrSemicolonEntityName, 5, DarrSemicolonEntityValue, 1 },
    { DashvSemicolonEntityName, 6, DashvSemicolonEntityValue, 1 },
    { DcaronSemicolonEntityName, 7, DcaronSemicolonEntityValue, 1 },
    { DcySemicolonEntityName, 4, DcySemicolonEntityValue, 1 },
    { DelSemicolonEntityName, 4, DelSemicolonEntityValue, 1 },
    { DeltaSemicolonEntityName, 6, DeltaSemicolonEntityValue, 1 },
    { DfrSemicolonEntityName, 4, DfrSemicolonEntityValue, 1 },
    { DiacriticalAcuteSemicolonEntityName, 17, DiacriticalAcuteSemicolonEntityValue, 1 },
    { DiacriticalDotSemicolonEntityName, 15, DiacriticalDotSemicolonEntityValue, 1 },
    { DiacriticalDoubleAcuteSemicolonEntityName, 23, DiacriticalDoubleAcuteSemicolonEntityValue, 1 },
    { DiacriticalGraveSemicolonEntityName, 17, DiacriticalGraveSemicolonEntityValue, 1 },
    { DiacriticalTildeSemicolonEntityName, 17, DiacriticalTildeSemicolonEntityValue, 1 },
    { DiamondSemicolonEntityName, 8, DiamondSemicolonEntityValue, 1 },
    { DifferentialDSemicolonEntityName, 14, DifferentialDSemicolonEntityValue, 1 },
    { DopfSemicolonEntityName, 5, DopfSemicolonEntityValue, 1 },
    { DotSemicolonEntityName, 4, DotSemicolonEntityValue, 1 },
    { DotDotSemicolonEntityName, 7, DotDotSemicolonEntityValue, 1 },
    { DotEqualSemicolonEntityName, 9, DotEqualSemicolonEntityValue, 1 },
    { DoubleContourIntegralSemicolonEntityName, 22, DoubleContourIntegralSemicolonEntityValue, 1 },
    { DoubleDotSemicolonEntityName, 10, DoubleDotSemicolonEntityValue, 1 },
    { DoubleDownArrowSemicolonEntityName, 16, DoubleDownArrowSemicolonEntityValue, 1 },
    { DoubleLeftArrowSemicolonEntityName, 16, DoubleLeftArrowSemicolonEntityValue, 1 },
    { DoubleLeftRightArrowSemicolonEntityName, 21, DoubleLeftRightArrowSemicolonEntityValue, 1 },
    { DoubleLeftTeeSemicolonEntityName, 14, DoubleLeftTeeSemicolonEntityValue, 1 },
    { DoubleLongLeftArrowSemicolonEntityName, 20, DoubleLongLeftArrowSemicolonEntityValue, 1 },
    { DoubleLongLeftRightArrowSemicolonEntityName, 25, DoubleLongLeftRightArrowSemicolonEntityValue, 1 },
    { DoubleLongRightArrowSemicolonEntityName, 21, DoubleLongRightArrowSemicolonEntityValue, 1 },
    { DoubleRightArrowSemicolonEntityName, 17, DoubleRightArrowSemicolonEntityValue, 1 },
    { DoubleRightTeeSemicolonEntityName, 15, DoubleRightTeeSemicolonEntityValue, 1 },
    { DoubleUpArrowSemicolonEntityName, 14, DoubleUpArrowSemicolonEntityValue, 1 },
    { DoubleUpDownArrowSemicolonEntityName, 18, DoubleUpDownArrowSemicolonEntityValue, 1 },
    { DoubleVerticalBarSemicolonEntityName, 18, DoubleVerticalBarSemicolonEntityValue, 1 },
    { DownArrowSemicolonEntityName, 10, DownArrowSemicolonEntityValue, 1 },
    { DownArrowBarSemicolonEntityName, 13, DownArrowBarSemicolonEntityValue, 1 },
    { DownArrowUpArrowSemicolonEntityName, 17, DownArrowUpArrowSemicolonEntityValue, 1 },
    { DownBreveSemicolonEntityName, 10, DownBreveSemicolonEntityValue, 1 },
    { DownLeftRightVectorSemicolonEntityName, 20, DownLeftRightVectorSemicolonEntityValue, 1 },
    { DownLeftTeeVectorSemicolonEntityName, 18, DownLeftTeeVectorSemicolonEntityValue, 1 },
    { DownLeftVectorSemicolonEntityName, 15, DownLeftVectorSemicolonEntityValue, 1 },
    { DownLeftVectorBarSemicolonEntityName, 18, DownLeftVectorBarSemicolonEntityValue, 1 },
    { DownRightTeeVectorSemicolonEntityName, 19, DownRightTeeVectorSemicolonEntityValue, 1 },
    { DownRightVectorSemicolonEntityName, 16, DownRightVectorSemicolonEntityValue, 1 },
    { DownRightVectorBarSemicolonEntityName, 19, DownRightVectorBarSemicolonEntityValue, 1 },
    { DownTeeSemicolonEntityName, 8, DownTeeSemicolonEntityValue, 1 },
    { DownTeeArrowSemicolonEntityName, 13, DownTeeArrowSemicolonEntityValue, 1 },
    { DownarrowSemicolonEntityName, 10, DownarrowSemicolonEntityValue, 1 },
    { DscrSemicolonEntityName, 5, DscrSemicolonEntityValue, 1 },
    { DstrokSemicolonEntityName, 7, DstrokSemicolonEntityValue, 1 },
    { ENGSemicolonEntityName, 4, ENGSemicolonEntityValue, 1 },
    { ETHEntityName, 3, ETHEntityValue, 1 },
    { ETHSemicolonEntityName, 4, ETHSemicolonEntityValue, 1 },
    { EacuteEntityName, 6, EacuteEntityValue, 1 },
    { EacuteSemicolonEntityName, 7, EacuteSemicolonEntityValue, 1 },
    { EcaronSemicolonEntityName, 7, EcaronSemicolonEntityValue, 1 },
    { EcircEntityName, 5, EcircEntityValue, 1 },
    { EcircSemicolonEntityName, 6, EcircSemicolonEntityValue, 1 },
    { EcySemicolonEntityName, 4, EcySemicolonEntityValue, 1 },
    { EdotSemicolonEntityName, 5, EdotSemicolonEntityValue, 1 },
    { EfrSemicolonEntityName, 4, EfrSemicolonEntityValue, 1 },
    { EgraveEntityName, 6, EgraveEntityValue, 1 },
    { EgraveSemicolonEntityName, 7, EgraveSemicolonEntityValue, 1 },
    { ElementSemicolonEntityName, 8, ElementSemicolonEntityValue, 1 },
    { EmacrSemicolonEntityName, 6, EmacrSemicolonEntityValue, 1 },
    { EmptySmallSquareSemicolonEntityName, 17, EmptySmallSquareSemicolonEntityValue, 1 },
    { EmptyVerySmallSquareSemicolonEntityName, 21, EmptyVerySmallSquareSemicolonEntityValue, 1 },
    { EogonSemicolonEntityName, 6, EogonSemicolonEntityValue, 1 },
    { EopfSemicolonEntityName, 5, EopfSemicolonEntityValue, 1 },
    { EpsilonSemicolonEntityName, 8, EpsilonSemicolonEntityValue, 1 },
    { EqualSemicolonEntityName, 6, EqualSemicolonEntityValue, 1 },
    { EqualTildeSemicolonEntityName, 11, EqualTildeSemicolonEntityValue, 1 },
    { EquilibriumSemicolonEntityName, 12, EquilibriumSemicolonEntityValue, 1 },
    { EscrSemicolonEntityName, 5, EscrSemicolonEntityValue, 1 },
    { EsimSemicolonEntityName, 5, EsimSemicolonEntityValue, 1 },
    { EtaSemicolonEntityName, 4, EtaSemicolonEntityValue, 1 },
    { EumlEntityName, 4, EumlEntityValue, 1 },
    { EumlSemicolonEntityName, 5, EumlSemicolonEntityValue, 1 },
    { ExistsSemicolonEntityName, 7, ExistsSemicolonEntityValue, 1 },
    { ExponentialESemicolonEntityName, 13, ExponentialESemicolonEntityValue, 1 },
    { FcySemicolonEntityName, 4, FcySemicolonEntityValue, 1 },
    { FfrSemicolonEntityName, 4, FfrSemicolonEntityValue, 1 },
    { FilledSmallSquareSemicolonEntityName, 18, FilledSmallSquareSemicolonEntityValue, 1 },
    { FilledVerySmallSquareSemicolonEntityName, 22, FilledVerySmallSquareSemicolonEntityValue, 1 },
    { FopfSemicolonEntityName, 5, FopfSemicolonEntityValue, 1 },
    { ForAllSemicolonEntityName, 7, ForAllSemicolonEntityValue, 1 },
    { FouriertrfSemicolonEntityName, 11, FouriertrfSemicolonEntityValue, 1 },
    { FscrSemicolonEntityName, 5, FscrSemicolonEntityValue, 1 },
    { GJcySemicolonEntityName, 5, GJcySemicolonEntityValue, 1 },
    { GTEntityName, 2, GTEntityValue, 1 },
    { GTSemicolonEntityName, 3, GTSemicolonEntityValue, 1 },
    { GammaSemicolonEntityName, 6, GammaSemicolonEntityValue, 1 },
    { GammadSemicolonEntityName, 7, GammadSemicolonEntityValue, 1 },
    { GbreveSemicolonEntityName, 7, GbreveSemicolonEntityValue, 1 },
    { GcedilSemicolonEntityName, 7, GcedilSemicolonEntityValue, 1 },
    { GcircSemicolonEntityName, 6, GcircSemicolonEntityValue, 1 },
    { GcySemicolonEntityName, 4, GcySemicolonEntityValue, 1 },
    { GdotSemicolonEntityName, 5, GdotSemicolonEntityValue, 1 },
    { GfrSemicolonEntityName, 4, GfrSemicolonEntityValue, 1 },
    { GgSemicolonEntityName, 3, GgSemicolonEntityValue, 1 },
    { GopfSemicolonEntityName, 5, GopfSemicolonEntityValue, 1 },
    { GreaterEqualSemicolonEntityName, 13, GreaterEqualSemicolonEntityValue, 1 },
    { GreaterEqualLessSemicolonEntityName, 17, GreaterEqualLessSemicolonEntityValue, 1 },
    { GreaterFullEqualSemicolonEntityName, 17, GreaterFullEqualSemicolonEntityValue, 1 },
    { GreaterGreaterSemicolonEntityName, 15, GreaterGreaterSemicolonEntityValue, 1 },
    { GreaterLessSemicolonEntityName, 12, GreaterLessSemicolonEntityValue, 1 },
    { GreaterSlantEqualSemicolonEntityName, 18, GreaterSlantEqualSemicolonEntityValue, 1 },
    { GreaterTildeSemicolonEntityName, 13, GreaterTildeSemicolonEntityValue, 1 },
    { GscrSemicolonEntityName, 5, GscrSemicolonEntityValue, 1 },
    { GtSemicolonEntityName, 3, GtSemicolonEntityValue, 1 },
    { HARDcySemicolonEntityName, 7, HARDcySemicolonEntityValue, 1 },
    { HacekSemicolonEntityName, 6, HacekSemicolonEntityValue, 1 },
    { HatSemicolonEntityName, 4, HatSemicolonEntityValue, 1 },
    { HcircSemicolonEntityName, 6, HcircSemicolonEntityValue, 1 },
    { HfrSemicolonEntityName, 4, HfrSemicolonEntityValue, 1 },
    { HilbertSpaceSemicolonEntityName, 13, HilbertSpaceSemicolonEntityValue, 1 },
    { HopfSemicolonEntityName, 5, HopfSemicolonEntityValue, 1 },
    { HorizontalLineSemicolonEntityName, 15, HorizontalLineSemicolonEntityValue, 1 },
    { HscrSemicolonEntityName, 5, HscrSemicolonEntityValue, 1 },
    { HstrokSemicolonEntityName, 7, HstrokSemicolonEntityValue, 1 },
    { HumpDownHumpSemicolonEntityName, 13, HumpDownHumpSemicolonEntityValue, 1 },
    { HumpEqualSemicolonEntityName, 10, HumpEqualSemicolonEntityValue, 1 },
    { IEcySemicolonEntityName, 5, IEcySemicolonEntityValue, 1 },
    { IJligSemicolonEntityName, 6, IJligSemicolonEntityValue, 1 },
    { IOcySemicolonEntityName, 5, IOcySemicolonEntityValue, 1 },
    { IacuteEntityName, 6, IacuteEntityValue, 1 },
    { IacuteSemicolonEntityName, 7, IacuteSemicolonEntityValue, 1 },
    { IcircEntityName, 5, IcircEntityValue, 1 },
    { IcircSemicolonEntityName, 6, IcircSemicolonEntityValue, 1 },
    { IcySemicolonEntityName, 4, IcySemicolonEntityValue, 1 },
    { IdotSemicolonEntityName, 5, IdotSemicolonEntityValue, 1 },
    { IfrSemicolonEntityName, 4, IfrSemicolonEntityValue, 1 },
    { IgraveEntityName, 6, IgraveEntityValue, 1 },
    { IgraveSemicolonEntityName, 7, IgraveSemicolonEntityValue, 1 },
    { ImSemicolonEntityName, 3, ImSemicolonEntityValue, 1 },
    { ImacrSemicolonEntityName, 6, ImacrSemicolonEntityValue, 1 },
    { ImaginaryISemicolonEntityName, 11, ImaginaryISemicolonEntityValue, 1 },
    { ImpliesSemicolonEntityName, 8, ImpliesSemicolonEntityValue, 1 },
    { IntSemicolonEntityName, 4, IntSemicolonEntityValue, 1 },
    { IntegralSemicolonEntityName, 9, IntegralSemicolonEntityValue, 1 },
    { IntersectionSemicolonEntityName, 13, IntersectionSemicolonEntityValue, 1 },
    { InvisibleCommaSemicolonEntityName, 15, InvisibleCommaSemicolonEntityValue, 1 },
    { InvisibleTimesSemicolonEntityName, 15, InvisibleTimesSemicolonEntityValue, 1 },
    { IogonSemicolonEntityName, 6, IogonSemicolonEntityValue, 1 },
    { IopfSemicolonEntityName, 5, IopfSemicolonEntityValue, 1 },
    { IotaSemicolonEntityName, 5, IotaSemicolonEntityValue, 1 },
    { IscrSemicolonEntityName, 5, IscrSemicolonEntityValue, 1 },
    { ItildeSemicolonEntityName, 7, ItildeSemicolonEntityValue, 1 },
    { IukcySemicolonEntityName, 6, IukcySemicolonEntityValue, 1 },
    { IumlEntityName, 4, IumlEntityValue, 1 },
    { IumlSemicolonEntityName, 5, IumlSemicolonEntityValue, 1 },
    { JcircSemicolonEntityName, 6, JcircSemicolonEntityValue, 1 },
    { JcySemicolonEntityName, 4, JcySemicolonEntityValue, 1 },
    { JfrSemicolonEntityName, 4, JfrSemicolonEntityValue, 1 },
    { JopfSemicolonEntityName, 5, JopfSemicolonEntityValue, 1 },
    { JscrSemicolonEntityName, 5, JscrSemicolonEntityValue, 1 },
    { JsercySemicolonEntityName, 7, JsercySemicolonEntityValue, 1 },
    { JukcySemicolonEntityName, 6, JukcySemicolonEntityValue, 1 },
    { KHcySemicolonEntityName, 5, KHcySemicolonEntityValue, 1 },
    { KJcySemicolonEntityName, 5, KJcySemicolonEntityValue, 1 },
    { KappaSemicolonEntityName, 6, KappaSemicolonEntityValue, 1 },
    { KcedilSemicolonEntityName, 7, KcedilSemicolonEntityValue, 1 },
    { KcySemicolonEntityName, 4, KcySemicolonEntityValue, 1 },
    { KfrSemicolonEntityName, 4, KfrSemicolonEntityValue, 1 },
    { KopfSemicolonEntityName, 5, KopfSemicolonEntityValue, 1 },
    { KscrSemicolonEntityName, 5, KscrSemicolonEntityValue, 1 },
    { LJcySemicolonEntityName, 5, LJcySemicolonEntityValue, 1 },
    { LTEntityName, 2, LTEntityValue, 1 },
    { LTSemicolonEntityName, 3, LTSemicolonEntityValue, 1 },
    { LacuteSemicolonEntityName, 7, LacuteSemicolonEntityValue, 1 },
    { LambdaSemicolonEntityName, 7, LambdaSemicolonEntityValue, 1 },
    { LangSemicolonEntityName, 5, LangSemicolonEntityValue, 1 },
    { LaplacetrfSemicolonEntityName, 11, LaplacetrfSemicolonEntityValue, 1 },
    { LarrSemicolonEntityName, 5, LarrSemicolonEntityValue, 1 },
    { LcaronSemicolonEntityName, 7, LcaronSemicolonEntityValue, 1 },
    { LcedilSemicolonEntityName, 7, LcedilSemicolonEntityValue, 1 },
    { LcySemicolonEntityName, 4, LcySemicolonEntityValue, 1 },
    { LeftAngleBracketSemicolonEntityName, 17, LeftAngleBracketSemicolonEntityValue, 1 },
    { LeftArrowSemicolonEntityName, 10, LeftArrowSemicolonEntityValue, 1 },
    { LeftArrowBarSemicolonEntityName, 13, LeftArrowBarSemicolonEntityValue, 1 },
    { LeftArrowRightArrowSemicolonEntityName, 20, LeftArrowRightArrowSemicolonEntityValue, 1 },
    { LeftCeilingSemicolonEntityName, 12, LeftCeilingSemicolonEntityValue, 1 },
    { LeftDoubleBracketSemicolonEntityName, 18, LeftDoubleBracketSemicolonEntityValue, 1 },
    { LeftDownTeeVectorSemicolonEntityName, 18, LeftDownTeeVectorSemicolonEntityValue, 1 },
    { LeftDownVectorSemicolonEntityName, 15, LeftDownVectorSemicolonEntityValue, 1 },
    { LeftDownVectorBarSemicolonEntityName, 18, LeftDownVectorBarSemicolonEntityValue, 1 },
    { LeftFloorSemicolonEntityName, 10, LeftFloorSemicolonEntityValue, 1 },
    { LeftRightArrowSemicolonEntityName, 15, LeftRightArrowSemicolonEntityValue, 1 },
    { LeftRightVectorSemicolonEntityName, 16, LeftRightVectorSemicolonEntityValue, 1 },
    { LeftTeeSemicolonEntityName, 8, LeftTeeSemicolonEntityValue, 1 },
    { LeftTeeArrowSemicolonEntityName, 13, LeftTeeArrowSemicolonEntityValue, 1 },
    { LeftTeeVectorSemicolonEntityName, 14, LeftTeeVectorSemicolonEntityValue, 1 },
    { LeftTriangleSemicolonEntityName, 13, LeftTriangleSemicolonEntityValue, 1 },
    { LeftTriangleBarSemicolonEntityName, 16, LeftTriangleBarSemicolonEntityValue, 1 },
    { LeftTriangleEqualSemicolonEntityName, 18, LeftTriangleEqualSemicolonEntityValue, 1 },
    { LeftUpDownVectorSemicolonEntityName, 17, LeftUpDownVectorSemicolonEntityValue, 1 },
    { LeftUpTeeVectorSemicolonEntityName, 16, LeftUpTeeVectorSemicolonEntityValue, 1 },
    { LeftUpVectorSemicolonEntityName, 13, LeftUpVectorSemicolonEntityValue, 1 },
    { LeftUpVectorBarSemicolonEntityName, 16, LeftUpVectorBarSemicolonEntityValue, 1 },
    { LeftVectorSemicolonEntityName, 11, LeftVectorSemicolonEntityValue, 1 },
    { LeftVectorBarSemicolonEntityName, 14, LeftVectorBarSemicolonEntityValue, 1 },
    { LeftarrowSemicolonEntityName, 10, LeftarrowSemicolonEntityValue, 1 },
    { LeftrightarrowSemicolonEntityName, 15, LeftrightarrowSemicolonEntityValue, 1 },
    { LessEqualGreaterSemicolonEntityName, 17, LessEqualGreaterSemicolonEntityValue, 1 },
    { LessFullEqualSemicolonEntityName, 14, LessFullEqualSemicolonEntityValue, 1 },
    { LessGreaterSemicolonEntityName, 12, LessGreaterSemicolonEntityValue, 1 },
    { LessLessSemicolonEntityName, 9, LessLessSemicolonEntityValue, 1 },
    { LessSlantEqualSemicolonEntityName, 15, LessSlantEqualSemicolonEntityValue, 1 },
    { LessTildeSemicolonEntityName, 10, LessTildeSemicolonEntityValue, 1 },
    { LfrSemicolonEntityName, 4, LfrSemicolonEntityValue, 1 },
    { LlSemicolonEntityName, 3, LlSemicolonEntityValue, 1 },
    { LleftarrowSemicolonEntityName, 11, LleftarrowSemicolonEntityValue, 1 },
    { LmidotSemicolonEntityName, 7, LmidotSemicolonEntityValue, 1 },
    { LongLeftArrowSemicolonEntityName, 14, LongLeftArrowSemicolonEntityValue, 1 },
    { LongLeftRightArrowSemicolonEntityName, 19, LongLeftRightArrowSemicolonEntityValue, 1 },
    { LongRightArrowSemicolonEntityName, 15, LongRightArrowSemicolonEntityValue, 1 },
    { LongleftarrowSemicolonEntityName, 14, LongleftarrowSemicolonEntityValue, 1 },
    { LongleftrightarrowSemicolonEntityName, 19, LongleftrightarrowSemicolonEntityValue, 1 },
    { LongrightarrowSemicolonEntityName, 15, LongrightarrowSemicolonEntityValue, 1 },
    { LopfSemicolonEntityName, 5, LopfSemicolonEntityValue, 1 },
    { LowerLeftArrowSemicolonEntityName, 15, LowerLeftArrowSemicolonEntityValue, 1 },
    { LowerRightArrowSemicolonEntityName, 16, LowerRightArrowSemicolonEntityValue, 1 },
    { LscrSemicolonEntityName, 5, LscrSemicolonEntityValue, 1 },
    { LshSemicolonEntityName, 4, LshSemicolonEntityValue, 1 },
    { LstrokSemicolonEntityName, 7, LstrokSemicolonEntityValue, 1 },
    { LtSemicolonEntityName, 3, LtSemicolonEntityValue, 1 },
    { MapSemicolonEntityName, 4, MapSemicolonEntityValue, 1 },
    { McySemicolonEntityName, 4, McySemicolonEntityValue, 1 },
    { MediumSpaceSemicolonEntityName, 12, MediumSpaceSemicolonEntityValue, 1 },
    { MellintrfSemicolonEntityName, 10, MellintrfSemicolonEntityValue, 1 },
    { MfrSemicolonEntityName, 4, MfrSemicolonEntityValue, 1 },
    { MinusPlusSemicolonEntityName, 10, MinusPlusSemicolonEntityValue, 1 },
    { MopfSemicolonEntityName, 5, MopfSemicolonEntityValue, 1 },
    { MscrSemicolonEntityName, 5, MscrSemicolonEntityValue, 1 },
    { MuSemicolonEntityName, 3, MuSemicolonEntityValue, 1 },
    { NJcySemicolonEntityName, 5, NJcySemicolonEntityValue, 1 },
    { NacuteSemicolonEntityName, 7, NacuteSemicolonEntityValue, 1 },
    { NcaronSemicolonEntityName, 7, NcaronSemicolonEntityValue, 1 },
    { NcedilSemicolonEntityName, 7, NcedilSemicolonEntityValue, 1 },
    { NcySemicolonEntityName, 4, NcySemicolonEntityValue, 1 },
    { NegativeMediumSpaceSemicolonEntityName, 20, NegativeMediumSpaceSemicolonEntityValue, 1 },
    { NegativeThickSpaceSemicolonEntityName, 19, NegativeThickSpaceSemicolonEntityValue, 1 },
    { NegativeThinSpaceSemicolonEntityName, 18, NegativeThinSpaceSemicolonEntityValue, 1 },
    { NegativeVeryThinSpaceSemicolonEntityName, 22, NegativeVeryThinSpaceSemicolonEntityValue, 1 },
    { NestedGreaterGreaterSemicolonEntityName, 21, NestedGreaterGreaterSemicolonEntityValue, 1 },
    { NestedLessLessSemicolonEntityName, 15, NestedLessLessSemicolonEntityValue, 1 },
    { NewLineSemicolonEntityName, 8, NewLineSemicolonEntityValue, 1 },
    { NfrSemicolonEntityName, 4, NfrSemicolonEntityValue, 1 },
    { NoBreakSemicolonEntityName, 8, NoBreakSemicolonEntityValue, 1 },
    { NonBreakingSpaceSemicolonEntityName, 17, NonBreakingSpaceSemicolonEntityValue, 1 },
    { NopfSemicolonEntityName, 5, NopfSemicolonEntityValue, 1 },
    { NotSemicolonEntityName, 4, NotSemicolonEntityValue, 1 },
    { NotCongruentSemicolonEntityName, 13, NotCongruentSemicolonEntityValue, 1 },
    { NotCupCapSemicolonEntityName, 10, NotCupCapSemicolonEntityValue, 1 },
    { NotDoubleVerticalBarSemicolonEntityName, 21, NotDoubleVerticalBarSemicolonEntityValue, 1 },
    { NotElementSemicolonEntityName, 11, NotElementSemicolonEntityValue, 1 },
    { NotEqualSemicolonEntityName, 9, NotEqualSemicolonEntityValue, 1 },
    { NotEqualTildeSemicolonEntityName, 14, NotEqualTildeSemicolonEntityValue, 2 },
    { NotExistsSemicolonEntityName, 10, NotExistsSemicolonEntityValue, 1 },
    { NotGreaterSemicolonEntityName, 11, NotGreaterSemicolonEntityValue, 1 },
    { NotGreaterEqualSemicolonEntityName, 16, NotGreaterEqualSemicolonEntityValue, 1 },
    { NotGreaterFullEqualSemicolonEntityName, 20, NotGreaterFullEqualSemicolonEntityValue, 2 },
    { NotGreaterGreaterSemicolonEntityName, 18, NotGreaterGreaterSemicolonEntityValue, 2 },
    { NotGreaterLessSemicolonEntityName, 15, NotGreaterLessSemicolonEntityValue, 1 },
    { NotGreaterSlantEqualSemicolonEntityName, 21, NotGreaterSlantEqualSemicolonEntityValue, 2 },
    { NotGreaterTildeSemicolonEntityName, 16, NotGreaterTildeSemicolonEntityValue, 1 },
    { NotHumpDownHumpSemicolonEntityName, 16, NotHumpDownHumpSemicolonEntityValue, 2 },
    { NotHumpEqualSemicolonEntityName, 13, NotHumpEqualSemicolonEntityValue, 2 },
    { NotLeftTriangleSemicolonEntityName, 16, NotLeftTriangleSemicolonEntityValue, 1 },
    { NotLeftTriangleBarSemicolonEntityName, 19, NotLeftTriangleBarSemicolonEntityValue, 2 },
    { NotLeftTriangleEqualSemicolonEntityName, 21, NotLeftTriangleEqualSemicolonEntityValue, 1 },
    { NotLessSemicolonEntityName, 8, NotLessSemicolonEntityValue, 1 },
    { NotLessEqualSemicolonEntityName, 13, NotLessEqualSemicolonEntityValue, 1 },
    { NotLessGreaterSemicolonEntityName, 15, NotLessGreaterSemicolonEntityValue, 1 },
    { NotLessLessSemicolonEntityName, 12, NotLessLessSemicolonEntityValue, 2 },
    { NotLessSlantEqualSemicolonEntityName, 18, NotLessSlantEqualSemicolonEntityValue, 2 },
    { NotLessTildeSemicolonEntityName, 13, NotLessTildeSemicolonEntityValue, 1 },
    { NotNestedGreaterGreaterSemicolonEntityName, 24, NotNestedGreaterGreaterSemicolonEntityValue, 2 },
    { NotNestedLessLessSemicolonEntityName, 18, NotNestedLessLessSemicolonEntityValue, 2 },
    { NotPrecedesSemicolonEntityName, 12, NotPrecedesSemicolonEntityValue, 1 },
    { NotPrecedesEqualSemicolonEntityName, 17, NotPrecedesEqualSemicolonEntityValue, 2 },
    { NotPrecedesSlantEqualSemicolonEntityName, 22, NotPrecedesSlantEqualSemicolonEntityValue, 1 },
    { NotReverseElementSemicolonEntityName, 18, NotReverseElementSemicolonEntityValue, 1 },
    { NotRightTriangleSemicolonEntityName, 17, NotRightTriangleSemicolonEntityValue, 1 },
    { NotRightTriangleBarSemicolonEntityName, 20, NotRightTriangleBarSemicolonEntityValue, 2 },
    { NotRightTriangleEqualSemicolonEntityName, 22, NotRightTriangleEqualSemicolonEntityValue, 1 },
    { NotSquareSubsetSemicolonEntityName, 16, NotSquareSubsetSemicolonEntityValue, 2 },
    { NotSquareSubsetEqualSemicolonEntityName, 21, NotSquareSubsetEqualSemicolonEntityValue, 1 },
    { NotSquareSupersetSemicolonEntityName, 18, NotSquareSupersetSemicolonEntityValue, 2 },
    { NotSquareSupersetEqualSemicolonEntityName, 23, NotSquareSupersetEqualSemicolonEntityValue, 1 },
    { NotSubsetSemicolonEntityName, 10, NotSubsetSemicolonEntityValue, 2 },
    { NotSubsetEqualSemicolonEntityName, 15, NotSubsetEqualSemicolonEntityValue, 1 },
    { NotSucceedsSemicolonEntityName, 12, NotSucceedsSemicolonEntityValue, 1 },
    { NotSucceedsEqualSemicolonEntityName, 17, NotSucceedsEqualSemicolonEntityValue, 2 },
    { NotSucceedsSlantEqualSemicolonEntityName, 22, NotSucceedsSlantEqualSemicolonEntityValue, 1 },
    { NotSucceedsTildeSemicolonEntityName, 17, NotSucceedsTildeSemicolonEntityValue, 2 },
    { NotSupersetSemicolonEntityName, 12, NotSupersetSemicolonEntityValue, 2 },
    { NotSupersetEqualSemicolonEntityName, 17, NotSupersetEqualSemicolonEntityValue, 1 },
    { NotTildeSemicolonEntityName, 9, NotTildeSemicolonEntityValue, 1 },
    { NotTildeEqualSemicolonEntityName, 14, NotTildeEqualSemicolonEntityValue, 1 },
    { NotTildeFullEqualSemicolonEntityName, 18, NotTildeFullEqualSemicolonEntityValue, 1 },
    { NotTildeTildeSemicolonEntityName, 14, NotTildeTildeSemicolonEntityValue, 1 },
    { NotVerticalBarSemicolonEntityName, 15, NotVerticalBarSemicolonEntityValue, 1 },
    { NscrSemicolonEntityName, 5, NscrSemicolonEntityValue, 1 },
    { NtildeEntityName, 6, NtildeEntityValue, 1 },
    { NtildeSemicolonEntityName, 7, NtildeSemicolonEntityValue, 1 },
    { NuSemicolonEntityName, 3, NuSemicolonEntityValue, 1 },
    { OEligSemicolonEntityName, 6, OEligSemicolonEntityValue, 1 },
    { OacuteEntityName, 6, OacuteEntityValue, 1 },
    { OacuteSemicolonEntityName, 7, OacuteSemicolonEntityValue, 1 },
    { OcircEntityName, 5, OcircEntityValue, 1 },
    { OcircSemicolonEntityName, 6, OcircSemicolonEntityValue, 1 },
    { OcySemicolonEntityName, 4, OcySemicolonEntityValue, 1 },
    { OdblacSemicolonEntityName, 7, OdblacSemicolonEntityValue, 1 },
    { OfrSemicolonEntityName, 4, OfrSemicolonEntityValue, 1 },
    { OgraveEntityName, 6, OgraveEntityValue, 1 },
    { OgraveSemicolonEntityName, 7, OgraveSemicolonEntityValue, 1 },
    { OmacrSemicolonEntityName, 6, OmacrSemicolonEntityValue, 1 },
    { OmegaSemicolonEntityName, 6, OmegaSemicolonEntityValue, 1 },
    { OmicronSemicolonEntityName, 8, OmicronSemicolonEntityValue, 1 },
    { OopfSemicolonEntityName, 5, OopfSemicolonEntityValue, 1 },
    { OpenCurlyDoubleQuoteSemicolonEntityName, 21, OpenCurlyDoubleQuoteSemicolonEntityValue, 1 },
    { OpenCurlyQuoteSemicolonEntityName, 15, OpenCurlyQuoteSemicolonEntityValue, 1 },
    { OrSemicolonEntityName, 3, OrSemicolonEntityValue, 1 },
    { OscrSemicolonEntityName, 5, OscrSemicolonEntityValue, 1 },
    { OslashEntityName, 6, OslashEntityValue, 1 },
    { OslashSemicolonEntityName, 7, OslashSemicolonEntityValue, 1 },
    { OtildeEntityName, 6, OtildeEntityValue, 1 },
    { OtildeSemicolonEntityName, 7, OtildeSemicolonEntityValue, 1 },
    { OtimesSemicolonEntityName, 7, OtimesSemicolonEntityValue, 1 },
    { OumlEntityName, 4, OumlEntityValue, 1 },
    { OumlSemicolonEntityName, 5, OumlSemicolonEntityValue, 1 },
    { OverBarSemicolonEntityName, 8, OverBarSemicolonEntityValue, 1 },
    { OverBraceSemicolonEntityName, 10, OverBraceSemicolonEntityValue, 1 },
    { OverBracketSemicolonEntityName, 12, OverBracketSemicolonEntityValue, 1 },
    { OverParenthesisSemicolonEntityName, 16, OverParenthesisSemicolonEntityValue, 1 },
    { PartialDSemicolonEntityName, 9, PartialDSemicolonEntityValue, 1 },
    { PcySemicolonEntityName, 4, PcySemicolonEntityValue, 1 },
    { PfrSemicolonEntityName, 4, PfrSemicolonEntityValue, 1 },
    { PhiSemicolonEntityName, 4, PhiSemicolonEntityValue, 1 },
    { PiSemicolonEntityName, 3, PiSemicolonEntityValue, 1 },
    { PlusMinusSemicolonEntityName, 10, PlusMinusSemicolonEntityValue, 1 },
    { PoincareplaneSemicolonEntityName, 14, PoincareplaneSemicolonEntityValue, 1 },
    { PopfSemicolonEntityName, 5, PopfSemicolonEntityValue, 1 },
    { PrSemicolonEntityName, 3, PrSemicolonEntityValue, 1 },
    { PrecedesSemicolonEntityName, 9, PrecedesSemicolonEntityValue, 1 },
    { PrecedesEqualSemicolonEntityName, 14, PrecedesEqualSemicolonEntityValue, 1 },
    { PrecedesSlantEqualSemicolonEntityName, 19, PrecedesSlantEqualSemicolonEntityValue, 1 },
    { PrecedesTildeSemicolonEntityName, 14, PrecedesTildeSemicolonEntityValue, 1 },
    { PrimeSemicolonEntityName, 6, PrimeSemicolonEntityValue, 1 },
    { ProductSemicolonEntityName, 8, ProductSemicolonEntityValue, 1 },
    { ProportionSemicolonEntityName, 11, ProportionSemicolonEntityValue, 1 },
    { ProportionalSemicolonEntityName, 13, ProportionalSemicolonEntityValue, 1 },
    { PscrSemicolonEntityName, 5, PscrSemicolonEntityValue, 1 },
    { PsiSemicolonEntityName, 4, PsiSemicolonEntityValue, 1 },
    { QUOTEntityName, 4, QUOTEntityValue, 1 },
    { QUOTSemicolonEntityName, 5, QUOTSemicolonEntityValue, 1 },
    { QfrSemicolonEntityName, 4, QfrSemicolonEntityValue, 1 },
    { QopfSemicolonEntityName, 5, QopfSemicolonEntityValue, 1 },
    { QscrSemicolonEntityName, 5, QscrSemicolonEntityValue, 1 },
    { RBarrSemicolonEntityName, 6, RBarrSemicolonEntityValue, 1 },
    { REGEntityName, 3, REGEntityValue, 1 },
    { REGSemicolonEntityName, 4, REGSemicolonEntityValue, 1 },
    { RacuteSemicolonEntityName, 7, RacuteSemicolonEntityValue, 1 },
    { RangSemicolonEntityName, 5, RangSemicolonEntityValue, 1 },
    { RarrSemicolonEntityName, 5, RarrSemicolonEntityValue, 1 },
    { RarrtlSemicolonEntityName, 7, RarrtlSemicolonEntityValue, 1 },
    { RcaronSemicolonEntityName, 7, RcaronSemicolonEntityValue, 1 },
    { RcedilSemicolonEntityName, 7, RcedilSemicolonEntityValue, 1 },
    { RcySemicolonEntityName, 4, RcySemicolonEntityValue, 1 },
    { ReSemicolonEntityName, 3, ReSemicolonEntityValue, 1 },
    { ReverseElementSemicolonEntityName, 15, ReverseElementSemicolonEntityValue, 1 },
    { ReverseEquilibriumSemicolonEntityName, 19, ReverseEquilibriumSemicolonEntityValue, 1 },
    { ReverseUpEquilibriumSemicolonEntityName, 21, ReverseUpEquilibriumSemicolonEntityValue, 1 },
    { RfrSemicolonEntityName, 4, RfrSemicolonEntityValue, 1 },
    { RhoSemicolonEntityName, 4, RhoSemicolonEntityValue, 1 },
    { RightAngleBracketSemicolonEntityName, 18, RightAngleBracketSemicolonEntityValue, 1 },
    { RightArrowSemicolonEntityName, 11, RightArrowSemicolonEntityValue, 1 },
    { RightArrowBarSemicolonEntityName, 14, RightArrowBarSemicolonEntityValue, 1 },
    { RightArrowLeftArrowSemicolonEntityName, 20, RightArrowLeftArrowSemicolonEntityValue, 1 },
    { RightCeilingSemicolonEntityName, 13, RightCeilingSemicolonEntityValue, 1 },
    { RightDoubleBracketSemicolonEntityName, 19, RightDoubleBracketSemicolonEntityValue, 1 },
    { RightDownTeeVectorSemicolonEntityName, 19, RightDownTeeVectorSemicolonEntityValue, 1 },
    { RightDownVectorSemicolonEntityName, 16, RightDownVectorSemicolonEntityValue, 1 },
    { RightDownVectorBarSemicolonEntityName, 19, RightDownVectorBarSemicolonEntityValue, 1 },
    { RightFloorSemicolonEntityName, 11, RightFloorSemicolonEntityValue, 1 },
    { RightTeeSemicolonEntityName, 9, RightTeeSemicolonEntityValue, 1 },
    { RightTeeArrowSemicolonEntityName, 14, RightTeeArrowSemicolonEntityValue, 1 },
    { RightTeeVectorSemicolonEntityName, 15, RightTeeVectorSemicolonEntityValue, 1 },
    { RightTriangleSemicolonEntityName, 14, RightTriangleSemicolonEntityValue, 1 },
    { RightTriangleBarSemicolonEntityName, 17, RightTriangleBarSemicolonEntityValue, 1 },
    { RightTriangleEqualSemicolonEntityName, 19, RightTriangleEqualSemicolonEntityValue, 1 },
    { RightUpDownVectorSemicolonEntityName, 18, RightUpDownVectorSemicolonEntityValue, 1 },
    { RightUpTeeVectorSemicolonEntityName, 17, RightUpTeeVectorSemicolonEntityValue, 1 },
    { RightUpVectorSemicolonEntityName, 14, RightUpVectorSemicolonEntityValue, 1 },
    { RightUpVectorBarSemicolonEntityName, 17, RightUpVectorBarSemicolonEntityValue, 1 },
    { RightVectorSemicolonEntityName, 12, RightVectorSemicolonEntityValue, 1 },
    { RightVectorBarSemicolonEntityName, 15, RightVectorBarSemicolonEntityValue, 1 },
    { RightarrowSemicolonEntityName, 11, RightarrowSemicolonEntityValue, 1 },
    { RopfSemicolonEntityName, 5, RopfSemicolonEntityValue, 1 },
    { RoundImpliesSemicolonEntityName, 13, RoundImpliesSemicolonEntityValue, 1 },
    { RrightarrowSemicolonEntityName, 12, RrightarrowSemicolonEntityValue, 1 },
    { RscrSemicolonEntityName, 5, RscrSemicolonEntityValue, 1 },
    { RshSemicolonEntityName, 4, RshSemicolonEntityValue, 1 },
    { RuleDelayedSemicolonEntityName, 12, RuleDelayedSemicolonEntityValue, 1 },
    { SHCHcySemicolonEntityName, 7, SHCHcySemicolonEntityValue, 1 },
    { SHcySemicolonEntityName, 5, SHcySemicolonEntityValue, 1 },
    { SOFTcySemicolonEntityName, 7, SOFTcySemicolonEntityValue, 1 },
    { SacuteSemicolonEntityName, 7, SacuteSemicolonEntityValue, 1 },
    { ScSemicolonEntityName, 3, ScSemicolonEntityValue, 1 },
    { ScaronSemicolonEntityName, 7, ScaronSemicolonEntityValue, 1 },
    { ScedilSemicolonEntityName, 7, ScedilSemicolonEntityValue, 1 },
    { ScircSemicolonEntityName, 6, ScircSemicolonEntityValue, 1 },
    { ScySemicolonEntityName, 4, ScySemicolonEntityValue, 1 },
    { SfrSemicolonEntityName, 4, SfrSemicolonEntityValue, 1 },
    { ShortDownArrowSemicolonEntityName, 15, ShortDownArrowSemicolonEntityValue, 1 },
    { ShortLeftArrowSemicolonEntityName, 15, ShortLeftArrowSemicolonEntityValue, 1 },
    { ShortRightArrowSemicolonEntityName, 16, ShortRightArrowSemicolonEntityValue, 1 },
    { ShortUpArrowSemicolonEntityName, 13, ShortUpArrowSemicolonEntityValue, 1 },
    { SigmaSemicolonEntityName, 6, SigmaSemicolonEntityValue, 1 },
    { SmallCircleSemicolonEntityName, 12, SmallCircleSemicolonEntityValue, 1 },
    { SopfSemicolonEntityName, 5, SopfSemicolonEntityValue, 1 },
    { SqrtSemicolonEntityName, 5, SqrtSemicolonEntityValue, 1 },
    { SquareSemicolonEntityName, 7, SquareSemicolonEntityValue, 1 },
    { SquareIntersectionSemicolonEntityName, 19, SquareIntersectionSemicolonEntityValue, 1 },
    { SquareSubsetSemicolonEntityName, 13, SquareSubsetSemicolonEntityValue, 1 },
    { SquareSubsetEqualSemicolonEntityName, 18, SquareSubsetEqualSemicolonEntityValue, 1 },
    { SquareSupersetSemicolonEntityName, 15, SquareSupersetSemicolonEntityValue, 1 },
    { SquareSupersetEqualSemicolonEntityName, 20, SquareSupersetEqualSemicolonEntityValue, 1 },
    { SquareUnionSemicolonEntityName, 12, SquareUnionSemicolonEntityValue, 1 },
    { SscrSemicolonEntityName, 5, SscrSemicolonEntityValue, 1 },
    { StarSemicolonEntityName, 5, StarSemicolonEntityValue, 1 },
    { SubSemicolonEntityName, 4, SubSemicolonEntityValue, 1 },
    { SubsetSemicolonEntityName, 7, SubsetSemicolonEntityValue, 1 },
    { SubsetEqualSemicolonEntityName, 12, SubsetEqualSemicolonEntityValue, 1 },
    { SucceedsSemicolonEntityName, 9, SucceedsSemicolonEntityValue, 1 },
    { SucceedsEqualSemicolonEntityName, 14, SucceedsEqualSemicolonEntityValue, 1 },
    { SucceedsSlantEqualSemicolonEntityName, 19, SucceedsSlantEqualSemicolonEntityValue, 1 },
    { SucceedsTildeSemicolonEntityName, 14, SucceedsTildeSemicolonEntityValue, 1 },
    { SuchThatSemicolonEntityName, 9, SuchThatSemicolonEntityValue, 1 },
    { SumSemicolonEntityName, 4, SumSemicolonEntityValue, 1 },
    { SupSemicolonEntityName, 4, SupSemicolonEntityValue, 1 },
    { SupersetSemicolonEntityName, 9, SupersetSemicolonEntityValue, 1 },
    { SupersetEqualSemicolonEntityName, 14, SupersetEqualSemicolonEntityValue, 1 },
    { SupsetSemicolonEntityName, 7, SupsetSemicolonEntityValue, 1 },
    { THORNEntityName, 5, THORNEntityValue, 1 },
    { THORNSemicolonEntityName, 6, THORNSemicolonEntityValue, 1 },
    { TRADESemicolonEntityName, 6, TRADESemicolonEntityValue, 1 },
    { TSHcySemicolonEntityName, 6, TSHcySemicolonEntityValue, 1 },
    { TScySemicolonEntityName, 5, TScySemicolonEntityValue, 1 },
    { TabSemicolonEntityName, 4, TabSemicolonEntityValue, 1 },
    { TauSemicolonEntityName, 4, TauSemicolonEntityValue, 1 },
    { TcaronSemicolonEntityName, 7, TcaronSemicolonEntityValue, 1 },
    { TcedilSemicolonEntityName, 7, TcedilSemicolonEntityValue, 1 },
    { TcySemicolonEntityName, 4, TcySemicolonEntityValue, 1 },
    { TfrSemicolonEntityName, 4, TfrSemicolonEntityValue, 1 },
    { ThereforeSemicolonEntityName, 10, ThereforeSemicolonEntityValue, 1 },
    { ThetaSemicolonEntityName, 6, ThetaSemicolonEntityValue, 1 },
    { ThickSpaceSemicolonEntityName, 11, ThickSpaceSemicolonEntityValue, 2 },
    { ThinSpaceSemicolonEntityName, 10, ThinSpaceSemicolonEntityValue, 1 },
    { TildeSemicolonEntityName, 6, TildeSemicolonEntityValue, 1 },
    { TildeEqualSemicolonEntityName, 11, TildeEqualSemicolonEntityValue, 1 },
    { TildeFullEqualSemicolonEntityName, 15, TildeFullEqualSemicolonEntityValue, 1 },
    { TildeTildeSemicolonEntityName, 11, TildeTildeSemicolonEntityValue, 1 },
    { TopfSemicolonEntityName, 5, TopfSemicolonEntityValue, 1 },
    { TripleDotSemicolonEntityName, 10, TripleDotSemicolonEntityValue, 1 },
    { TscrSemicolonEntityName, 5, TscrSemicolonEntityValue, 1 },
    { TstrokSemicolonEntityName, 7, TstrokSemicolonEntityValue, 1 },
    { UacuteEntityName, 6, UacuteEntityValue, 1 },
    { UacuteSemicolonEntityName, 7, UacuteSemicolonEntityValue, 1 },
    { UarrSemicolonEntityName, 5, UarrSemicolonEntityValue, 1 },
    { UarrocirSemicolonEntityName, 9, UarrocirSemicolonEntityValue, 1 },
    { UbrcySemicolonEntityName, 6, UbrcySemicolonEntityValue, 1 },
    { UbreveSemicolonEntityName, 7, UbreveSemicolonEntityValue, 1 },
    { UcircEntityName, 5, UcircEntityValue, 1 },
    { UcircSemicolonEntityName, 6, UcircSemicolonEntityValue, 1 },
    { UcySemicolonEntityName, 4, UcySemicolonEntityValue, 1 },
    { UdblacSemicolonEntityName, 7, UdblacSemicolonEntityValue, 1 },
    { UfrSemicolonEntityName, 4, UfrSemicolonEntityValue, 1 },
    { UgraveEntityName, 6, UgraveEntityValue, 1 },
    { UgraveSemicolonEntityName, 7, UgraveSemicolonEntityValue, 1 },
    { UmacrSemicolonEntityName, 6, UmacrSemicolonEntityValue, 1 },
    { UnderBarSemicolonEntityName, 9, UnderBarSemicolonEntityValue, 1 },
    { UnderBraceSemicolonEntityName, 11, UnderBraceSemicolonEntityValue, 1 },
    { UnderBracketSemicolonEntityName, 13, UnderBracketSemicolonEntityValue, 1 },
    { UnderParenthesisSemicolonEntityName, 17, UnderParenthesisSemicolonEntityValue, 1 },
    { UnionSemicolonEntityName, 6, UnionSemicolonEntityValue, 1 },
    { UnionPlusSemicolonEntityName, 10, UnionPlusSemicolonEntityValue, 1 },
    { UogonSemicolonEntityName, 6, UogonSemicolonEntityValue, 1 },
    { UopfSemicolonEntityName, 5, UopfSemicolonEntityValue, 1 },
    { UpArrowSemicolonEntityName, 8, UpArrowSemicolonEntityValue, 1 },
    { UpArrowBarSemicolonEntityName, 11, UpArrowBarSemicolonEntityValue, 1 },
    { UpArrowDownArrowSemicolonEntityName, 17, UpArrowDownArrowSemicolonEntityValue, 1 },
    { UpDownArrowSemicolonEntityName, 12, UpDownArrowSemicolonEntityValue, 1 },
    { UpEquilibriumSemicolonEntityName, 14, UpEquilibriumSemicolonEntityValue, 1 },
    { UpTeeSemicolonEntityName, 6, UpTeeSemicolonEntityValue, 1 },
    { UpTeeArrowSemicolonEntityName, 11, UpTeeArrowSemicolonEntityValue, 1 },
    { UparrowSemicolonEntityName, 8, UparrowSemicolonEntityValue, 1 },
    { UpdownarrowSemicolonEntityName, 12, UpdownarrowSemicolonEntityValue, 1 },
    { UpperLeftArrowSemicolonEntityName, 15, UpperLeftArrowSemicolonEntityValue, 1 },
    { UpperRightArrowSemicolonEntityName, 16, UpperRightArrowSemicolonEntityValue, 1 },
    { UpsiSemicolonEntityName, 5, UpsiSemicolonEntityValue, 1 },
    { UpsilonSemicolonEntityName, 8, UpsilonSemicolonEntityValue, 1 },
    { UringSemicolonEntityName, 6, UringSemicolonEntityValue, 1 },
    { UscrSemicolonEntityName, 5, UscrSemicolonEntityValue, 1 },
    { UtildeSemicolonEntityName, 7, UtildeSemicolonEntityValue, 1 },
    { UumlEntityName, 4, UumlEntityValue, 1 },
    { UumlSemicolonEntityName, 5, UumlSemicolonEntityValue, 1 },
    { VDashSemicolonEntityName, 6, VDashSemicolonEntityValue, 1 },
    { VbarSemicolonEntityName, 5, VbarSemicolonEntityValue, 1 },
    { VcySemicolonEntityName, 4, VcySemicolonEntityValue, 1 },
    { VdashSemicolonEntityName, 6, VdashSemicolonEntityValue, 1 },
    { VdashlSemicolonEntityName, 7, VdashlSemicolonEntityValue, 1 },
    { VeeSemicolonEntityName, 4, VeeSemicolonEntityValue, 1 },
    { VerbarSemicolonEntityName, 7, VerbarSemicolonEntityValue, 1 },
    { VertSemicolonEntityName, 5, VertSemicolonEntityValue, 1 },
    { VerticalBarSemicolonEntityName, 12, VerticalBarSemicolonEntityValue, 1 },
    { VerticalLineSemicolonEntityName, 13, VerticalLineSemicolonEntityValue, 1 },
    { VerticalSeparatorSemicolonEntityName, 18, VerticalSeparatorSemicolonEntityValue, 1 },
    { VerticalTildeSemicolonEntityName, 14, VerticalTildeSemicolonEntityValue, 1 },
    { VeryThinSpaceSemicolonEntityName, 14, VeryThinSpaceSemicolonEntityValue, 1 },
    { VfrSemicolonEntityName, 4, VfrSemicolonEntityValue, 1 },
    { VopfSemicolonEntityName, 5, VopfSemicolonEntityValue, 1 },
    { VscrSemicolonEntityName, 5, VscrSemicolonEntityValue, 1 },
    { VvdashSemicolonEntityName, 7, VvdashSemicolonEntityValue, 1 },
    { WcircSemicolonEntityName, 6, WcircSemicolonEntityValue, 1 },
    { WedgeSemicolonEntityName, 6, WedgeSemicolonEntityValue, 1 },
    { WfrSemicolonEntityName, 4, WfrSemicolonEntityValue, 1 },
    { WopfSemicolonEntityName, 5, WopfSemicolonEntityValue, 1 },
    { WscrSemicolonEntityName, 5, WscrSemicolonEntityValue, 1 },
    { XfrSemicolonEntityName, 4, XfrSemicolonEntityValue, 1 },
    { XiSemicolonEntityName, 3, XiSemicolonEntityValue, 1 },
    { XopfSemicolonEntityName, 5, XopfSemicolonEntityValue, 1 },
    { XscrSemicolonEntityName, 5, XscrSemicolonEntityValue, 1 },
    { YAcySemicolonEntityName, 5, YAcySemicolonEntityValue, 1 },
    { YIcySemicolonEntityName, 5, YIcySemicolonEntityValue, 1 },
    { YUcySemicolonEntityName, 5, YUcySemicolonEntityValue, 1 },
    { YacuteEntityName, 6, YacuteEntityValue, 1 },
    { YacuteSemicolonEntityName, 7, YacuteSemicolonEntityValue, 1 },
    { YcircSemicolonEntityName, 6, YcircSemicolonEntityValue, 1 },
    { YcySemicolonEntityName, 4, YcySemicolonEntityValue, 1 },
    { YfrSemicolonEntityName, 4, YfrSemicolonEntityValue, 1 },
    { YopfSemicolonEntityName, 5, YopfSemicolonEntityValue, 1 },
    { YscrSemicolonEntityName, 5, YscrSemicolonEntityValue, 1 },
    { YumlSemicolonEntityName, 5, YumlSemicolonEntityValue, 1 },
    { ZHcySemicolonEntityName, 5, ZHcySemicolonEntityValue, 1 },
    { ZacuteSemicolonEntityName, 7, ZacuteSemicolonEntityValue, 1 },
    { ZcaronSemicolonEntityName, 7, ZcaronSemicolonEntityValue, 1 },
    { ZcySemicolonEntityName, 4, ZcySemicolonEntityValue, 1 },
    { ZdotSemicolonEntityName, 5, ZdotSemicolonEntityValue, 1 },
    { ZeroWidthSpaceSemicolonEntityName, 15, ZeroWidthSpaceSemicolonEntityValue, 1 },
    { ZetaSemicolonEntityName, 5, ZetaSemicolonEntityValue, 1 },
    { ZfrSemicolonEntityName, 4, ZfrSemicolonEntityValue, 1 },
    { ZopfSemicolonEntityName, 5, ZopfSemicolonEntityValue, 1 },
    { ZscrSemicolonEntityName, 5, ZscrSemicolonEntityValue, 1 },
    { aacuteEntityName, 6, aacuteEntityValue, 1 },
    { aacuteSemicolonEntityName, 7, aacuteSemicolonEntityValue, 1 },
    { abreveSemicolonEntityName, 7, abreveSemicolonEntityValue, 1 },
    { acSemicolonEntityName, 3, acSemicolonEntityValue, 1 },
    { acESemicolonEntityName, 4, acESemicolonEntityValue, 2 },
    { acdSemicolonEntityName, 4, acdSemicolonEntityValue, 1 },
    { acircEntityName, 5, acircEntityValue, 1 },
    { acircSemicolonEntityName, 6, acircSemicolonEntityValue, 1 },
    { acuteEntityName, 5, acuteEntityValue, 1 },
    { acuteSemicolonEntityName, 6, acuteSemicolonEntityValue, 1 },
    { acySemicolonEntityName, 4, acySemicolonEntityValue, 1 },
    { aeligEntityName, 5, aeligEntityValue, 1 },
    { aeligSemicolonEntityName, 6, aeligSemicolonEntityValue, 1 },
    { afSemicolonEntityName, 3, afSemicolonEntityValue, 1 },
    { afrSemicolonEntityName, 4, afrSemicolonEntityValue, 1 },
    { agraveEntityName, 6, agraveEntityValue, 1 },
    { agraveSemicolonEntityName, 7, agraveSemicolonEntityValue, 1 },
    { alefsymSemicolonEntityName, 8, alefsymSemicolonEntityValue, 1 },
    { alephSemicolonEntityName, 6, alephSemicolonEntityValue, 1 },
    { alphaSemicolonEntityName, 6, alphaSemicolonEntityValue, 1 },
    { amacrSemicolonEntityName, 6, amacrSemicolonEntityValue, 1 },
    { amalgSemicolonEntityName, 6, amalgSemicolonEntityValue, 1 },
    { ampEntityName, 3, ampEntityValue, 1 },
    { ampSemicolonEntityName, 4, ampSemicolonEntityValue, 1 },
    { andSemicolonEntityName, 4, andSemicolonEntityValue, 1 },
    { andandSemicolonEntityName, 7, andandSemicolonEntityValue, 1 },
    { anddSemicolonEntityName, 5, anddSemicolonEntityValue, 1 },
    { andslopeSemicolonEntityName, 9, andslopeSemicolonEntityValue, 1 },
    { andvSemicolonEntityName, 5, andvSemicolonEntityValue, 1 },
    { angSemicolonEntityName, 4, angSemicolonEntityValue, 1 },
    { angeSemicolonEntityName, 5, angeSemicolonEntityValue, 1 },
    { angleSemicolonEntityName, 6, angleSemicolonEntityValue, 1 },
    { angmsdSemicolonEntityName, 7, angmsdSemicolonEntityValue, 1 },
    { angmsdaaSemicolonEntityName, 9, angmsdaaSemicolonEntityValue, 1 },
    { angmsdabSemicolonEntityName, 9, angmsdabSemicolonEntityValue, 1 },
    { angmsdacSemicolonEntityName, 9, angmsdacSemicolonEntityValue, 1 },
    { angmsdadSemicolonEntityName, 9, angmsdadSemicolonEntityValue, 1 },
    { angmsdaeSemicolonEntityName, 9, angmsdaeSemicolonEntityValue, 1 },
    { angmsdafSemicolonEntityName, 9, angmsdafSemicolonEntityValue, 1 },
    { angmsdagSemicolonEntityName, 9, angmsdagSemicolonEntityValue, 1 },
    { angmsdahSemicolonEntityName, 9, angmsdahSemicolonEntityValue, 1 },
    { angrtSemicolonEntityName, 6, angrtSemicolonEntityValue, 1 },
    { angrtvbSemicolonEntityName, 8, angrtvbSemicolonEntityValue, 1 },
    { angrtvbdSemicolonEntityName, 9, angrtvbdSemicolonEntityValue, 1 },
    { angsphSemicolonEntityName, 7, angsphSemicolonEntityValue, 1 },
    { angstSemicolonEntityName, 6, angstSemicolonEntityValue, 1 },
    { angzarrSemicolonEntityName, 8, angzarrSemicolonEntityValue, 1 },
    { aogonSemicolonEntityName, 6, aogonSemicolonEntityValue, 1 },
    { aopfSemicolonEntityName, 5, aopfSemicolonEntityValue, 1 },
    { apSemicolonEntityName, 3, apSemicolonEntityValue, 1 },
    { apESemicolonEntityName, 4, apESemicolonEntityValue, 1 },
    { apacirSemicolonEntityName, 7, apacirSemicolonEntityValue, 1 },
    { apeSemicolonEntityName, 4, apeSemicolonEntityValue, 1 },
    { apidSemicolonEntityName, 5, apidSemicolonEntityValue, 1 },
    { aposSemicolonEntityName, 5, aposSemicolonEntityValue, 1 },
    { approxSemicolonEntityName, 7, approxSemicolonEntityValue, 1 },
    { approxeqSemicolonEntityName, 9, approxeqSemicolonEntityValue, 1 },
    { aringEntityName, 5, aringEntityValue, 1 },
    { aringSemicolonEntityName, 6, aringSemicolonEntityValue, 1 },
    { ascrSemicolonEntityName, 5, ascrSemicolonEntityValue, 1 },
    { astSemicolonEntityName, 4, astSemicolonEntityValue, 1 },
    { asympSemicolonEntityName, 6, asympSemicolonEntityValue, 1 },
    { asympeqSemicolonEntityName, 8, asympeqSemicolonEntityValue, 1 },
    { atildeEntityName, 6, atildeEntityValue, 1 },
    { atildeSemicolonEntityName, 7, atildeSemicolonEntityValue, 1 },
    { aumlEntityName, 4, aumlEntityValue, 1 },
    { aumlSemicolonEntityName, 5, aumlSemicolonEntityValue, 1 },
    { awconintSemicolonEntityName, 9, awconintSemicolonEntityValue, 1 },
    { awintSemicolonEntityName, 6, awintSemicolonEntityValue, 1 },
    { bNotSemicolonEntityName, 5, bNotSemicolonEntityValue, 1 },
    { backcongSemicolonEntityName, 9, backcongSemicolonEntityValue, 1 },
    { backepsilonSemicolonEntityName, 12, backepsilonSemicolonEntityValue, 1 },
    { backprimeSemicolonEntityName, 10, backprimeSemicolonEntityValue, 1 },
    { backsimSemicolonEntityName, 8, backsimSemicolonEntityValue, 1 },
    { backsimeqSemicolonEntityName, 10, backsimeqSemicolonEntityValue, 1 },
    { barveeSemicolonEntityName, 7, barveeSemicolonEntityValue, 1 },
    { barwedSemicolonEntityName, 7, barwedSemicolonEntityValue, 1 },
    { barwedgeSemicolonEntityName, 9, barwedgeSemicolonEntityValue, 1 },
    { bbrkSemicolonEntityName, 5, bbrkSemicolonEntityValue, 1 },
    { bbrktbrkSemicolonEntityName, 9, bbrktbrkSemicolonEntityValue, 1 },
    { bcongSemicolonEntityName, 6, bcongSemicolonEntityValue, 1 },
    { bcySemicolonEntityName, 4, bcySemicolonEntityValue, 1 },
    { bdquoSemicolonEntityName, 6, bdquoSemicolonEntityValue, 1 },
    { becausSemicolonEntityName, 7, becausSemicolonEntityValue, 1 },
    { becauseSemicolonEntityName, 8, becauseSemicolonEntityValue, 1 },
    { bemptyvSemicolonEntityName, 8, bemptyvSemicolonEntityValue, 1 },
    { bepsiSemicolonEntityName, 6, bepsiSemicolonEntityValue, 1 },
    { bernouSemicolonEntityName, 7, bernouSemicolonEntityValue, 1 },
    { betaSemicolonEntityName, 5, betaSemicolonEntityValue, 1 },
    { bethSemicolonEntityName, 5, bethSemicolonEntityValue, 1 },
    { betweenSemicolonEntityName, 8, betweenSemicolonEntityValue, 1 },
    { bfrSemicolonEntityName, 4, bfrSemicolonEntityValue, 1 },
    { bigcapSemicolonEntityName, 7, bigcapSemicolonEntityValue, 1 },
    { bigcircSemicolonEntityName, 8, bigcircSemicolonEntityValue, 1 },
    { bigcupSemicolonEntityName, 7, bigcupSemicolonEntityValue, 1 },
    { bigodotSemicolonEntityName, 8, bigodotSemicolonEntityValue, 1 },
    { bigoplusSemicolonEntityName, 9, bigoplusSemicolonEntityValue, 1 },
    { bigotimesSemicolonEntityName, 10, bigotimesSemicolonEntityValue, 1 },
    { bigsqcupSemicolonEntityName, 9, bigsqcupSemicolonEntityValue, 1 },
    { bigstarSemicolonEntityName, 8, bigstarSemicolonEntityValue, 1 },
    { bigtriangledownSemicolonEntityName, 16, bigtriangledownSemicolonEntityValue, 1 },
    { bigtriangleupSemicolonEntityName, 14, bigtriangleupSemicolonEntityValue, 1 },
    { biguplusSemicolonEntityName, 9, biguplusSemicolonEntityValue, 1 },
    { bigveeSemicolonEntityName, 7, bigveeSemicolonEntityValue, 1 },
    { bigwedgeSemicolonEntityName, 9, bigwedgeSemicolonEntityValue, 1 },
    { bkarowSemicolonEntityName, 7, bkarowSemicolonEntityValue, 1 },
    { blacklozengeSemicolonEntityName, 13, blacklozengeSemicolonEntityValue, 1 },
    { blacksquareSemicolonEntityName, 12, blacksquareSemicolonEntityValue, 1 },
    { blacktriangleSemicolonEntityName, 14, blacktriangleSemicolonEntityValue, 1 },
    { blacktriangledownSemicolonEntityName, 18, blacktriangledownSemicolonEntityValue, 1 },
    { blacktriangleleftSemicolonEntityName, 18, blacktriangleleftSemicolonEntityValue, 1 },
    { blacktrianglerightSemicolonEntityName, 19, blacktrianglerightSemicolonEntityValue, 1 },
    { blankSemicolonEntityName, 6, blankSemicolonEntityValue, 1 },
    { blk12SemicolonEntityName, 6, blk12SemicolonEntityValue, 1 },
    { blk14SemicolonEntityName, 6, blk14SemicolonEntityValue, 1 },
    { blk34SemicolonEntityName, 6, blk34SemicolonEntityValue, 1 },
    { blockSemicolonEntityName, 6, blockSemicolonEntityValue, 1 },
    { bneSemicolonEntityName, 4, bneSemicolonEntityValue, 2 },
    { bnequivSemicolonEntityName, 8, bnequivSemicolonEntityValue, 2 },
    { bnotSemicolonEntityName, 5, bnotSemicolonEntityValue, 1 },
    { bopfSemicolonEntityName, 5, bopfSemicolonEntityValue, 1 },
    { botSemicolonEntityName, 4, botSemicolonEntityValue, 1 },
    { bottomSemicolonEntityName, 7, bottomSemicolonEntityValue, 1 },
    { bowtieSemicolonEntityName, 7, bowtieSemicolonEntityValue, 1 },
    { boxDLSemicolonEntityName, 6, boxDLSemicolonEntityValue, 1 },
    { boxDRSemicolonEntityName, 6, boxDRSemicolonEntityValue, 1 },
    { boxDlSemicolonEntityName, 6, boxDlSemicolonEntityValue, 1 },
    { boxDrSemicolonEntityName, 6, boxDrSemicolonEntityValue, 1 },
    { boxHSemicolonEntityName, 5, boxHSemicolonEntityValue, 1 },
    { boxHDSemicolonEntityName, 6, boxHDSemicolonEntityValue, 1 },
    { boxHUSemicolonEntityName, 6, boxHUSemicolonEntityValue, 1 },
    { boxHdSemicolonEntityName, 6, boxHdSemicolonEntityValue, 1 },
    { boxHuSemicolonEntityName, 6, boxHuSemicolonEntityValue, 1 },
    { boxULSemicolonEntityName, 6, boxULSemicolonEntityValue, 1 },
    { boxURSemicolonEntityName, 6, boxURSemicolonEntityValue, 1 },
    { boxUlSemicolonEntityName, 6, boxUlSemicolonEntityValue, 1 },
    { boxUrSemicolonEntityName, 6, boxUrSemicolonEntityValue, 1 },
    { boxVSemicolonEntityName, 5, boxVSemicolonEntityValue, 1 },
    { boxVHSemicolonEntityName, 6, boxVHSemicolonEntityValue, 1 },
    { boxVLSemicolonEntityName, 6, boxVLSemicolonEntityValue, 1 },
    { boxVRSemicolonEntityName, 6, boxVRSemicolonEntityValue, 1 },
    { boxVhSemicolonEntityName, 6, boxVhSemicolonEntityValue, 1 },
    { boxVlSemicolonEntityName, 6, boxVlSemicolonEntityValue, 1 },
    { boxVrSemicolonEntityName, 6, boxVrSemicolonEntityValue, 1 },
    { boxboxSemicolonEntityName, 7, boxboxSemicolonEntityValue, 1 },
    { boxdLSemicolonEntityName, 6, boxdLSemicolonEntityValue, 1 },
    { boxdRSemicolonEntityName, 6, boxdRSemicolonEntityValue, 1 },
    { boxdlSemicolonEntityName, 6, boxdlSemicolonEntityValue, 1 },
    { boxdrSemicolonEntityName, 6, boxdrSemicolonEntityValue, 1 },
    { boxhSemicolonEntityName, 5, boxhSemicolonEntityValue, 1 },
    { boxhDSemicolonEntityName, 6, boxhDSemicolonEntityValue, 1 },
    { boxhUSemicolonEntityName, 6, boxhUSemicolonEntityValue, 1 },
    { boxhdSemicolonEntityName, 6, boxhdSemicolonEntityValue, 1 },
    { boxhuSemicolonEntityName, 6, boxhuSemicolonEntityValue, 1 },
    { boxminusSemicolonEntityName, 9, boxminusSemicolonEntityValue, 1 },
    { boxplusSemicolonEntityName, 8, boxplusSemicolonEntityValue, 1 },
    { boxtimesSemicolonEntityName, 9, boxtimesSemicolonEntityValue, 1 },
    { boxuLSemicolonEntityName, 6, boxuLSemicolonEntityValue, 1 },
    { boxuRSemicolonEntityName, 6, boxuRSemicolonEntityValue, 1 },
    { boxulSemicolonEntityName, 6, boxulSemicolonEntityValue, 1 },
    { boxurSemicolonEntityName, 6, boxurSemicolonEntityValue, 1 },
    { boxvSemicolonEntityName, 5, boxvSemicolonEntityValue, 1 },
    { boxvHSemicolonEntityName, 6, boxvHSemicolonEntityValue, 1 },
    { boxvLSemicolonEntityName, 6, boxvLSemicolonEntityValue, 1 },
    { boxvRSemicolonEntityName, 6, boxvRSemicolonEntityValue, 1 },
    { boxvhSemicolonEntityName, 6, boxvhSemicolonEntityValue, 1 },
    { boxvlSemicolonEntityName, 6, boxvlSemicolonEntityValue, 1 },
    { boxvrSemicolonEntityName, 6, boxvrSemicolonEntityValue, 1 },
    { bprimeSemicolonEntityName, 7, bprimeSemicolonEntityValue, 1 },
    { breveSemicolonEntityName, 6, breveSemicolonEntityValue, 1 },
    { brvbarEntityName, 6, brvbarEntityValue, 1 },
    { brvbarSemicolonEntityName, 7, brvbarSemicolonEntityValue, 1 },
    { bscrSemicolonEntityName, 5, bscrSemicolonEntityValue, 1 },
    { bsemiSemicolonEntityName, 6, bsemiSemicolonEntityValue, 1 },
    { bsimSemicolonEntityName, 5, bsimSemicolonEntityValue, 1 },
    { bsimeSemicolonEntityName, 6, bsimeSemicolonEntityValue, 1 },
    { bsolSemicolonEntityName, 5, bsolSemicolonEntityValue, 1 },
    { bsolbSemicolonEntityName, 6, bsolbSemicolonEntityValue, 1 },
    { bsolhsubSemicolonEntityName, 9, bsolhsubSemicolonEntityValue, 1 },
    { bullSemicolonEntityName, 5, bullSemicolonEntityValue, 1 },
    { bulletSemicolonEntityName, 7, bulletSemicolonEntityValue, 1 },
    { bumpSemicolonEntityName, 5, bumpSemicolonEntityValue, 1 },
    { bumpESemicolonEntityName, 6, bumpESemicolonEntityValue, 1 },
    { bumpeSemicolonEntityName, 6, bumpeSemicolonEntityValue, 1 },
    { bumpeqSemicolonEntityName, 7, bumpeqSemicolonEntityValue, 1 },
    { cacuteSemicolonEntityName, 7, cacuteSemicolonEntityValue, 1 },
    { capSemicolonEntityName, 4, capSemicolonEntityValue, 1 },
    { capandSemicolonEntityName, 7, capandSemicolonEntityValue, 1 },
    { capbrcupSemicolonEntityName, 9, capbrcupSemicolonEntityValue, 1 },
    { capcapSemicolonEntityName, 7, capcapSemicolonEntityValue, 1 },
    { capcupSemicolonEntityName, 7, capcupSemicolonEntityValue, 1 },
    { capdotSemicolonEntityName, 7, capdotSemicolonEntityValue, 1 },
    { capsSemicolonEntityName, 5, capsSemicolonEntityValue, 2 },
    { caretSemicolonEntityName, 6, caretSemicolonEntityValue, 1 },
    { caronSemicolonEntityName, 6, caronSemicolonEntityValue, 1 },
    { ccapsSemicolonEntityName, 6, ccapsSemicolonEntityValue, 1 },
    { ccaronSemicolonEntityName, 7, ccaronSemicolonEntityValue, 1 },
    { ccedilEntityName, 6, ccedilEntityValue, 1 },
    { ccedilSemicolonEntityName, 7, ccedilSemicolonEntityValue, 1 },
    { ccircSemicolonEntityName, 6, ccircSemicolonEntityValue, 1 },
    { ccupsSemicolonEntityName, 6, ccupsSemicolonEntityValue, 1 },
    { ccupssmSemicolonEntityName, 8, ccupssmSemicolonEntityValue, 1 },
    { cdotSemicolonEntityName, 5, cdotSemicolonEntityValue, 1 },
    { cedilEntityName, 5, cedilEntityValue, 1 },
    { cedilSemicolonEntityName, 6, cedilSemicolonEntityValue, 1 },
    { cemptyvSemicolonEntityName, 8, cemptyvSemicolonEntityValue, 1 },
    { centEntityName, 4, centEntityValue, 1 },
    { centSemicolonEntityName, 5, centSemicolonEntityValue, 1 },
    { centerdotSemicolonEntityName, 10, centerdotSemicolonEntityValue, 1 },
    { cfrSemicolonEntityName, 4, cfrSemicolonEntityValue, 1 },
    { chcySemicolonEntityName, 5, chcySemicolonEntityValue, 1 },
    { checkSemicolonEntityName, 6, checkSemicolonEntityValue, 1 },
    { checkmarkSemicolonEntityName, 10, checkmarkSemicolonEntityValue, 1 },
    { chiSemicolonEntityName, 4, chiSemicolonEntityValue, 1 },
    { cirSemicolonEntityName, 4, cirSemicolonEntityValue, 1 },
    { cirESemicolonEntityName, 5, cirESemicolonEntityValue, 1 },
    { circSemicolonEntityName, 5, circSemicolonEntityValue, 1 },
    { circeqSemicolonEntityName, 7, circeqSemicolonEntityValue, 1 },
    { circlearrowleftSemicolonEntityName, 16, circlearrowleftSemicolonEntityValue, 1 },
    { circlearrowrightSemicolonEntityName, 17, circlearrowrightSemicolonEntityValue, 1 },
    { circledRSemicolonEntityName, 9, circledRSemicolonEntityValue, 1 },
    { circledSSemicolonEntityName, 9, circledSSemicolonEntityValue, 1 },
    { circledastSemicolonEntityName, 11, circledastSemicolonEntityValue, 1 },
    { circledcircSemicolonEntityName, 12, circledcircSemicolonEntityValue, 1 },
    { circleddashSemicolonEntityName, 12, circleddashSemicolonEntityValue, 1 },
    { cireSemicolonEntityName, 5, cireSemicolonEntityValue, 1 },
    { cirfnintSemicolonEntityName, 9, cirfnintSemicolonEntityValue, 1 },
    { cirmidSemicolonEntityName, 7, cirmidSemicolonEntityValue, 1 },
    { cirscirSemicolonEntityName, 8, cirscirSemicolonEntityValue, 1 },
    { clubsSemicolonEntityName, 6, clubsSemicolonEntityValue, 1 },
    { clubsuitSemicolonEntityName, 9, clubsuitSemicolonEntityValue, 1 },
    { colonSemicolonEntityName, 6, colonSemicolonEntityValue, 1 },
    { coloneSemicolonEntityName, 7, coloneSemicolonEntityValue, 1 },
    { coloneqSemicolonEntityName, 8, coloneqSemicolonEntityValue, 1 },
    { commaSemicolonEntityName, 6, commaSemicolonEntityValue, 1 },
    { commatSemicolonEntityName, 7, commatSemicolonEntityValue, 1 },
    { compSemicolonEntityName, 5, compSemicolonEntityValue, 1 },
    { compfnSemicolonEntityName, 7, compfnSemicolonEntityValue, 1 },
    { complementSemicolonEntityName, 11, complementSemicolonEntityValue, 1 },
    { complexesSemicolonEntityName, 10, complexesSemicolonEntityValue, 1 },
    { congSemicolonEntityName, 5, congSemicolonEntityValue, 1 },
    { congdotSemicolonEntityName, 8, congdotSemicolonEntityValue, 1 },
    { conintSemicolonEntityName, 7, conintSemicolonEntityValue, 1 },
    { copfSemicolonEntityName, 5, copfSemicolonEntityValue, 1 },
    { coprodSemicolonEntityName, 7, coprodSemicolonEntityValue, 1 },
    { copyEntityName, 4, copyEntityValue, 1 },
    { copySemicolonEntityName, 5, copySemicolonEntityValue, 1 },
    { copysrSemicolonEntityName, 7, copysrSemicolonEntityValue, 1 },
    { crarrSemicolonEntityName, 6, crarrSemicolonEntityValue, 1 },
    { crossSemicolonEntityName, 6, crossSemicolonEntityValue, 1 },
    { cscrSemicolonEntityName, 5, cscrSemicolonEntityValue, 1 },
    { csubSemicolonEntityName, 5, csubSemicolonEntityValue, 1 },
    { csubeSemicolonEntityName, 6, csubeSemicolonEntityValue, 1 },
    { csupSemicolonEntityName, 5, csupSemicolonEntityValue, 1 },
    { csupeSemicolonEntityName, 6, csupeSemicolonEntityValue, 1 },
    { ctdotSemicolonEntityName, 6, ctdotSemicolonEntityValue, 1 },
    { cudarrlSemicolonEntityName, 8, cudarrlSemicolonEntityValue, 1 },
    { cudarrrSemicolonEntityName, 8, cudarrrSemicolonEntityValue, 1 },
    { cueprSemicolonEntityName, 6, cueprSemicolonEntityValue, 1 },
    { cuescSemicolonEntityName, 6, cuescSemicolonEntityValue, 1 },
    { cularrSemicolonEntityName, 7, cularrSemicolonEntityValue, 1 },
    { cularrpSemicolonEntityName, 8, cularrpSemicolonEntityValue, 1 },
    { cupSemicolonEntityName, 4, cupSemicolonEntityValue, 1 },
    { cupbrcapSemicolonEntityName, 9, cupbrcapSemicolonEntityValue, 1 },
    { cupcapSemicolonEntityName, 7, cupcapSemicolonEntityValue, 1 },
    { cupcupSemicolonEntityName, 7, cupcupSemicolonEntityValue, 1 },
    { cupdotSemicolonEntityName, 7, cupdotSemicolonEntityValue, 1 },
    { cuporSemicolonEntityName, 6, cuporSemicolonEntityValue, 1 },
    { cupsSemicolonEntityName, 5, cupsSemicolonEntityValue, 2 },
    { curarrSemicolonEntityName, 7, curarrSemicolonEntityValue, 1 },
    { curarrmSemicolonEntityName, 8, curarrmSemicolonEntityValue, 1 },
    { curlyeqprecSemicolonEntityName, 12, curlyeqprecSemicolonEntityValue, 1 },
    { curlyeqsuccSemicolonEntityName, 12, curlyeqsuccSemicolonEntityValue, 1 },
    { curlyveeSemicolonEntityName, 9, curlyveeSemicolonEntityValue, 1 },
    { curlywedgeSemicolonEntityName, 11, curlywedgeSemicolonEntityValue, 1 },
    { currenEntityName, 6, currenEntityValue, 1 },
    { currenSemicolonEntityName, 7, currenSemicolonEntityValue, 1 },
    { curvearrowleftSemicolonEntityName, 15, curvearrowleftSemicolonEntityValue, 1 },
    { curvearrowrightSemicolonEntityName, 16, curvearrowrightSemicolonEntityValue, 1 },
    { cuveeSemicolonEntityName, 6, cuveeSemicolonEntityValue, 1 },
    { cuwedSemicolonEntityName, 6, cuwedSemicolonEntityValue, 1 },
    { cwconintSemicolonEntityName, 9, cwconintSemicolonEntityValue, 1 },
    { cwintSemicolonEntityName, 6, cwintSemicolonEntityValue, 1 },
    { cylctySemicolonEntityName, 7, cylctySemicolonEntityValue, 1 },
    { dArrSemicolonEntityName, 5, dArrSemicolonEntityValue, 1 },
    { dHarSemicolonEntityName, 5, dHarSemicolonEntityValue, 1 },
    { daggerSemicolonEntityName, 7, daggerSemicolonEntityValue, 1 },
    { dalethSemicolonEntityName, 7, dalethSemicolonEntityValue, 1 },
    { darrSemicolonEntityName, 5, darrSemicolonEntityValue, 1 },
    { dashSemicolonEntityName, 5, dashSemicolonEntityValue, 1 },
    { dashvSemicolonEntityName, 6, dashvSemicolonEntityValue, 1 },
    { dbkarowSemicolonEntityName, 8, dbkarowSemicolonEntityValue, 1 },
    { dblacSemicolonEntityName, 6, dblacSemicolonEntityValue, 1 },
    { dcaronSemicolonEntityName, 7, dcaronSemicolonEntityValue, 1 },
    { dcySemicolonEntityName, 4, dcySemicolonEntityValue, 1 },
    { ddSemicolonEntityName, 3, ddSemicolonEntityValue, 1 },
    { ddaggerSemicolonEntityName, 8, ddaggerSemicolonEntityValue, 1 },
    { ddarrSemicolonEntityName, 6, ddarrSemicolonEntityValue, 1 },
    { ddotseqSemicolonEntityName, 8, ddotseqSemicolonEntityValue, 1 },
    { degEntityName, 3, degEntityValue, 1 },
    { degSemicolonEntityName, 4, degSemicolonEntityValue, 1 },
    { deltaSemicolonEntityName, 6, deltaSemicolonEntityValue, 1 },
    { demptyvSemicolonEntityName, 8, demptyvSemicolonEntityValue, 1 },
    { dfishtSemicolonEntityName, 7, dfishtSemicolonEntityValue, 1 },
    { dfrSemicolonEntityName, 4, dfrSemicolonEntityValue, 1 },
    { dharlSemicolonEntityName, 6, dharlSemicolonEntityValue, 1 },
    { dharrSemicolonEntityName, 6, dharrSemicolonEntityValue, 1 },
    { diamSemicolonEntityName, 5, diamSemicolonEntityValue, 1 },
    { diamondSemicolonEntityName, 8, diamondSemicolonEntityValue, 1 },
    { diamondsuitSemicolonEntityName, 12, diamondsuitSemicolonEntityValue, 1 },
    { diamsSemicolonEntityName, 6, diamsSemicolonEntityValue, 1 },
    { dieSemicolonEntityName, 4, dieSemicolonEntityValue, 1 },
    { digammaSemicolonEntityName, 8, digammaSemicolonEntityValue, 1 },
    { disinSemicolonEntityName, 6, disinSemicolonEntityValue, 1 },
    { divSemicolonEntityName, 4, divSemicolonEntityValue, 1 },
    { divideEntityName, 6, divideEntityValue, 1 },
    { divideSemicolonEntityName, 7, divideSemicolonEntityValue, 1 },
    { divideontimesSemicolonEntityName, 14, divideontimesSemicolonEntityValue, 1 },
    { divonxSemicolonEntityName, 7, divonxSemicolonEntityValue, 1 },
    { djcySemicolonEntityName, 5, djcySemicolonEntityValue, 1 },
    { dlcornSemicolonEntityName, 7, dlcornSemicolonEntityValue, 1 },
    { dlcropSemicolonEntityName, 7, dlcropSemicolonEntityValue, 1 },
    { dollarSemicolonEntityName, 7, dollarSemicolonEntityValue, 1 },
    { dopfSemicolonEntityName, 5, dopfSemicolonEntityValue, 1 },
    { dotSemicolonEntityName, 4, dotSemicolonEntityValue, 1 },
    { doteqSemicolonEntityName, 6, doteqSemicolonEntityValue, 1 },
    { doteqdotSemicolonEntityName, 9, doteqdotSemicolonEntityValue, 1 },
    { dotminusSemicolonEntityName, 9, dotminusSemicolonEntityValue, 1 },
    { dotplusSemicolonEntityName, 8, dotplusSemicolonEntityValue, 1 },
    { dotsquareSemicolonEntityName, 10, dotsquareSemicolonEntityValue, 1 },
    { doublebarwedgeSemicolonEntityName, 15, doublebarwedgeSemicolonEntityValue, 1 },
    { downarrowSemicolonEntityName, 10, downarrowSemicolonEntityValue, 1 },
    { downdownarrowsSemicolonEntityName, 15, downdownarrowsSemicolonEntityValue, 1 },
    { downharpoonleftSemicolonEntityName, 16, downharpoonleftSemicolonEntityValue, 1 },
    { downharpoonrightSemicolonEntityName, 17, downharpoonrightSemicolonEntityValue, 1 },
    { drbkarowSemicolonEntityName, 9, drbkarowSemicolonEntityValue, 1 },
    { drcornSemicolonEntityName, 7, drcornSemicolonEntityValue, 1 },
    { drcropSemicolonEntityName, 7, drcropSemicolonEntityValue, 1 },
    { dscrSemicolonEntityName, 5, dscrSemicolonEntityValue, 1 },
    { dscySemicolonEntityName, 5, dscySemicolonEntityValue, 1 },
    { dsolSemicolonEntityName, 5, dsolSemicolonEntityValue, 1 },
    { dstrokSemicolonEntityName, 7, dstrokSemicolonEntityValue, 1 },
    { dtdotSemicolonEntityName, 6, dtdotSemicolonEntityValue, 1 },
    { dtriSemicolonEntityName, 5, dtriSemicolonEntityValue, 1 },
    { dtrifSemicolonEntityName, 6, dtrifSemicolonEntityValue, 1 },
    { duarrSemicolonEntityName, 6, duarrSemicolonEntityValue, 1 },
    { duharSemicolonEntityName, 6, duharSemicolonEntityValue, 1 },
    { dwangleSemicolonEntityName, 8, dwangleSemicolonEntityValue, 1 },
    { dzcySemicolonEntityName, 5, dzcySemicolonEntityValue, 1 },
    { dzigrarrSemicolonEntityName, 9, dzigrarrSemicolonEntityValue, 1 },
    { eDDotSemicolonEntityName, 6, eDDotSemicolonEntityValue, 1 },
    { eDotSemicolonEntityName, 5, eDotSemicolonEntityValue, 1 },
    { eacuteEntityName, 6, eacuteEntityValue, 1 },
    { eacuteSemicolonEntityName, 7, eacuteSemicolonEntityValue, 1 },
    { easterSemicolonEntityName, 7, easterSemicolonEntityValue, 1 },
    { ecaronSemicolonEntityName, 7, ecaronSemicolonEntityValue, 1 },
    { ecirSemicolonEntityName, 5, ecirSemicolonEntityValue, 1 },
    { ecircEntityName, 5, ecircEntityValue, 1 },
    { ecircSemicolonEntityName, 6, ecircSemicolonEntityValue, 1 },
    { ecolonSemicolonEntityName, 7, ecolonSemicolonEntityValue, 1 },
    { ecySemicolonEntityName, 4, ecySemicolonEntityValue, 1 },
    { edotSemicolonEntityName, 5, edotSemicolonEntityValue, 1 },
    { eeSemicolonEntityName, 3, eeSemicolonEntityValue, 1 },
    { efDotSemicolonEntityName, 6, efDotSemicolonEntityValue, 1 },
    { efrSemicolonEntityName, 4, efrSemicolonEntityValue, 1 },
    { egSemicolonEntityName, 3, egSemicolonEntityValue, 1 },
    { egraveEntityName, 6, egraveEntityValue, 1 },
    { egraveSemicolonEntityName, 7, egraveSemicolonEntityValue, 1 },
    { egsSemicolonEntityName, 4, egsSemicolonEntityValue, 1 },
    { egsdotSemicolonEntityName, 7, egsdotSemicolonEntityValue, 1 },
    { elSemicolonEntityName, 3, elSemicolonEntityValue, 1 },
    { elintersSemicolonEntityName, 9, elintersSemicolonEntityValue, 1 },
    { ellSemicolonEntityName, 4, ellSemicolonEntityValue, 1 },
    { elsSemicolonEntityName, 4, elsSemicolonEntityValue, 1 },
    { elsdotSemicolonEntityName, 7, elsdotSemicolonEntityValue, 1 },
    { emacrSemicolonEntityName, 6, emacrSemicolonEntityValue, 1 },
    { emptySemicolonEntityName, 6, emptySemicolonEntityValue, 1 },
    { emptysetSemicolonEntityName, 9, emptysetSemicolonEntityValue, 1 },
    { emptyvSemicolonEntityName, 7, emptyvSemicolonEntityValue, 1 },
    { emsp13SemicolonEntityName, 7, emsp13SemicolonEntityValue, 1 },
    { emsp14SemicolonEntityName, 7, emsp14SemicolonEntityValue, 1 },
    { emspSemicolonEntityName, 5, emspSemicolonEntityValue, 1 },
    { engSemicolonEntityName, 4, engSemicolonEntityValue, 1 },
    { enspSemicolonEntityName, 5, enspSemicolonEntityValue, 1 },
    { eogonSemicolonEntityName, 6, eogonSemicolonEntityValue, 1 },
    { eopfSemicolonEntityName, 5, eopfSemicolonEntityValue, 1 },
    { eparSemicolonEntityName, 5, eparSemicolonEntityValue, 1 },
    { eparslSemicolonEntityName, 7, eparslSemicolonEntityValue, 1 },
    { eplusSemicolonEntityName, 6, eplusSemicolonEntityValue, 1 },
    { epsiSemicolonEntityName, 5, epsiSemicolonEntityValue, 1 },
    { epsilonSemicolonEntityName, 8, epsilonSemicolonEntityValue, 1 },
    { epsivSemicolonEntityName, 6, epsivSemicolonEntityValue, 1 },
    { eqcircSemicolonEntityName, 7, eqcircSemicolonEntityValue, 1 },
    { eqcolonSemicolonEntityName, 8, eqcolonSemicolonEntityValue, 1 },
    { eqsimSemicolonEntityName, 6, eqsimSemicolonEntityValue, 1 },
    { eqslantgtrSemicolonEntityName, 11, eqslantgtrSemicolonEntityValue, 1 },
    { eqslantlessSemicolonEntityName, 12, eqslantlessSemicolonEntityValue, 1 },
    { equalsSemicolonEntityName, 7, equalsSemicolonEntityValue, 1 },
    { equestSemicolonEntityName, 7, equestSemicolonEntityValue, 1 },
    { equivSemicolonEntityName, 6, equivSemicolonEntityValue, 1 },
    { equivDDSemicolonEntityName, 8, equivDDSemicolonEntityValue, 1 },
    { eqvparslSemicolonEntityName, 9, eqvparslSemicolonEntityValue, 1 },
    { erDotSemicolonEntityName, 6, erDotSemicolonEntityValue, 1 },
    { erarrSemicolonEntityName, 6, erarrSemicolonEntityValue, 1 },
    { escrSemicolonEntityName, 5, escrSemicolonEntityValue, 1 },
    { esdotSemicolonEntityName, 6, esdotSemicolonEntityValue, 1 },
    { esimSemicolonEntityName, 5, esimSemicolonEntityValue, 1 },
    { etaSemicolonEntityName, 4, etaSemicolonEntityValue, 1 },
    { ethEntityName, 3, ethEntityValue, 1 },
    { ethSemicolonEntityName, 4, ethSemicolonEntityValue, 1 },
    { eumlEntityName, 4, eumlEntityValue, 1 },
    { eumlSemicolonEntityName, 5, eumlSemicolonEntityValue, 1 },
    { euroSemicolonEntityName, 5, euroSemicolonEntityValue, 1 },
    { exclSemicolonEntityName, 5, exclSemicolonEntityValue, 1 },
    { existSemicolonEntityName, 6, existSemicolonEntityValue, 1 },
    { expectationSemicolonEntityName, 12, expectationSemicolonEntityValue, 1 },
    { exponentialeSemicolonEntityName, 13, exponentialeSemicolonEntityValue, 1 },
    { fallingdotseqSemicolonEntityName, 14, fallingdotseqSemicolonEntityValue, 1 },
    { fcySemicolonEntityName, 4, fcySemicolonEntityValue, 1 },
    { femaleSemicolonEntityName, 7, femaleSemicolonEntityValue, 1 },
    { ffiligSemicolonEntityName, 7, ffiligSemicolonEntityValue, 1 },
    { ffligSemicolonEntityName, 6, ffligSemicolonEntityValue, 1 },
    { fflligSemicolonEntityName, 7, fflligSemicolonEntityValue, 1 },
    { ffrSemicolonEntityName, 4, ffrSemicolonEntityValue, 1 },
    { filigSemicolonEntityName, 6, filigSemicolonEntityValue, 1 },
    { fjligSemicolonEntityName, 6, fjligSemicolonEntityValue, 2 },
    { flatSemicolonEntityName, 5, flatSemicolonEntityValue, 1 },
    { flligSemicolonEntityName, 6, flligSemicolonEntityValue, 1 },
    { fltnsSemicolonEntityName, 6, fltnsSemicolonEntityValue, 1 },
    { fnofSemicolonEntityName, 5, fnofSemicolonEntityValue, 1 },
    { fopfSemicolonEntityName, 5, fopfSemicolonEntityValue, 1 },
    { forallSemicolonEntityName, 7, forallSemicolonEntityValue, 1 },
    { forkSemicolonEntityName, 5, forkSemicolonEntityValue, 1 },
    { forkvSemicolonEntityName, 6, forkvSemicolonEntityValue, 1 },
    { fpartintSemicolonEntityName, 9, fpartintSemicolonEntityValue, 1 },
    { frac12EntityName, 6, frac12EntityValue, 1 },
    { frac12SemicolonEntityName, 7, frac12SemicolonEntityValue, 1 },
    { frac13SemicolonEntityName, 7, frac13SemicolonEntityValue, 1 },
    { frac14EntityName, 6, frac14EntityValue, 1 },
    { frac14SemicolonEntityName, 7, frac14SemicolonEntityValue, 1 },
    { frac15SemicolonEntityName, 7, frac15SemicolonEntityValue, 1 },
    { frac16SemicolonEntityName, 7, frac16SemicolonEntityValue, 1 },
    { frac18SemicolonEntityName, 7, frac18SemicolonEntityValue, 1 },
    { frac23SemicolonEntityName, 7, frac23SemicolonEntityValue, 1 },
    { frac25SemicolonEntityName, 7, frac25SemicolonEntityValue, 1 },
    { frac34EntityName, 6, frac34EntityValue, 1 },
    { frac34SemicolonEntityName, 7, frac34SemicolonEntityValue, 1 },
    { frac35SemicolonEntityName, 7, frac35SemicolonEntityValue, 1 },
    { frac38SemicolonEntityName, 7, frac38SemicolonEntityValue, 1 },
    { frac45SemicolonEntityName, 7, frac45SemicolonEntityValue, 1 },
    { frac56SemicolonEntityName, 7, frac56SemicolonEntityValue, 1 },
    { frac58SemicolonEntityName, 7, frac58SemicolonEntityValue, 1 },
    { frac78SemicolonEntityName, 7, frac78SemicolonEntityValue, 1 },
    { fraslSemicolonEntityName, 6, fraslSemicolonEntityValue, 1 },
    { frownSemicolonEntityName, 6, frownSemicolonEntityValue, 1 },
    { fscrSemicolonEntityName, 5, fscrSemicolonEntityValue, 1 },
    { gESemicolonEntityName, 3, gESemicolonEntityValue, 1 },
    { gElSemicolonEntityName, 4, gElSemicolonEntityValue, 1 },
    { gacuteSemicolonEntityName, 7, gacuteSemicolonEntityValue, 1 },
    { gammaSemicolonEntityName, 6, gammaSemicolonEntityValue, 1 },
    { gammadSemicolonEntityName, 7, gammadSemicolonEntityValue, 1 },
    { gapSemicolonEntityName, 4, gapSemicolonEntityValue, 1 },
    { gbreveSemicolonEntityName, 7, gbreveSemicolonEntityValue, 1 },
    { gcircSemicolonEntityName, 6, gcircSemicolonEntityValue, 1 },
    { gcySemicolonEntityName, 4, gcySemicolonEntityValue, 1 },
    { gdotSemicolonEntityName, 5, gdotSemicolonEntityValue, 1 },
    { geSemicolonEntityName, 3, geSemicolonEntityValue, 1 },
    { gelSemicolonEntityName, 4, gelSemicolonEntityValue, 1 },
    { geqSemicolonEntityName, 4, geqSemicolonEntityValue, 1 },
    { geqqSemicolonEntityName, 5, geqqSemicolonEntityValue, 1 },
    { geqslantSemicolonEntityName, 9, geqslantSemicolonEntityValue, 1 },
    { gesSemicolonEntityName, 4, gesSemicolonEntityValue, 1 },
    { gesccSemicolonEntityName, 6, gesccSemicolonEntityValue, 1 },
    { gesdotSemicolonEntityName, 7, gesdotSemicolonEntityValue, 1 },
    { gesdotoSemicolonEntityName, 8, gesdotoSemicolonEntityValue, 1 },
    { gesdotolSemicolonEntityName, 9, gesdotolSemicolonEntityValue, 1 },
    { geslSemicolonEntityName, 5, geslSemicolonEntityValue, 2 },
    { geslesSemicolonEntityName, 7, geslesSemicolonEntityValue, 1 },
    { gfrSemicolonEntityName, 4, gfrSemicolonEntityValue, 1 },
    { ggSemicolonEntityName, 3, ggSemicolonEntityValue, 1 },
    { gggSemicolonEntityName, 4, gggSemicolonEntityValue, 1 },
    { gimelSemicolonEntityName, 6, gimelSemicolonEntityValue, 1 },
    { gjcySemicolonEntityName, 5, gjcySemicolonEntityValue, 1 },
    { glSemicolonEntityName, 3, glSemicolonEntityValue, 1 },
    { glESemicolonEntityName, 4, glESemicolonEntityValue, 1 },
    { glaSemicolonEntityName, 4, glaSemicolonEntityValue, 1 },
    { gljSemicolonEntityName, 4, gljSemicolonEntityValue, 1 },
    { gnESemicolonEntityName, 4, gnESemicolonEntityValue, 1 },
    { gnapSemicolonEntityName, 5, gnapSemicolonEntityValue, 1 },
    { gnapproxSemicolonEntityName, 9, gnapproxSemicolonEntityValue, 1 },
    { gneSemicolonEntityName, 4, gneSemicolonEntityValue, 1 },
    { gneqSemicolonEntityName, 5, gneqSemicolonEntityValue, 1 },
    { gneqqSemicolonEntityName, 6, gneqqSemicolonEntityValue, 1 },
    { gnsimSemicolonEntityName, 6, gnsimSemicolonEntityValue, 1 },
    { gopfSemicolonEntityName, 5, gopfSemicolonEntityValue, 1 },
    { graveSemicolonEntityName, 6, graveSemicolonEntityValue, 1 },
    { gscrSemicolonEntityName, 5, gscrSemicolonEntityValue, 1 },
    { gsimSemicolonEntityName, 5, gsimSemicolonEntityValue, 1 },
    { gsimeSemicolonEntityName, 6, gsimeSemicolonEntityValue, 1 },
    { gsimlSemicolonEntityName, 6, gsimlSemicolonEntityValue, 1 },
    { gtEntityName, 2, gtEntityValue, 1 },
    { gtSemicolonEntityName, 3, gtSemicolonEntityValue, 1 },
    { gtccSemicolonEntityName, 5, gtccSemicolonEntityValue, 1 },
    { gtcirSemicolonEntityName, 6, gtcirSemicolonEntityValue, 1 },
    { gtdotSemicolonEntityName, 6, gtdotSemicolonEntityValue, 1 },
    { gtlParSemicolonEntityName, 7, gtlParSemicolonEntityValue, 1 },
    { gtquestSemicolonEntityName, 8, gtquestSemicolonEntityValue, 1 },
    { gtrapproxSemicolonEntityName, 10, gtrapproxSemicolonEntityValue, 1 },
    { gtrarrSemicolonEntityName, 7, gtrarrSemicolonEntityValue, 1 },
    { gtrdotSemicolonEntityName, 7, gtrdotSemicolonEntityValue, 1 },
    { gtreqlessSemicolonEntityName, 10, gtreqlessSemicolonEntityValue, 1 },
    { gtreqqlessSemicolonEntityName, 11, gtreqqlessSemicolonEntityValue, 1 },
    { gtrlessSemicolonEntityName, 8, gtrlessSemicolonEntityValue, 1 },
    { gtrsimSemicolonEntityName, 7, gtrsimSemicolonEntityValue, 1 },
    { gvertneqqSemicolonEntityName, 10, gvertneqqSemicolonEntityValue, 2 },
    { gvnESemicolonEntityName, 5, gvnESemicolonEntityValue, 2 },
    { hArrSemicolonEntityName, 5, hArrSemicolonEntityValue, 1 },
    { hairspSemicolonEntityName, 7, hairspSemicolonEntityValue, 1 },
    { halfSemicolonEntityName, 5, halfSemicolonEntityValue, 1 },
    { hamiltSemicolonEntityName, 7, hamiltSemicolonEntityValue, 1 },
    { hardcySemicolonEntityName, 7, hardcySemicolonEntityValue, 1 },
    { harrSemicolonEntityName, 5, harrSemicolonEntityValue, 1 },
    { harrcirSemicolonEntityName, 8, harrcirSemicolonEntityValue, 1 },
    { harrwSemicolonEntityName, 6, harrwSemicolonEntityValue, 1 },
    { hbarSemicolonEntityName, 5, hbarSemicolonEntityValue, 1 },
    { hcircSemicolonEntityName, 6, hcircSemicolonEntityValue, 1 },
    { heartsSemicolonEntityName, 7, heartsSemicolonEntityValue, 1 },
    { heartsuitSemicolonEntityName, 10, heartsuitSemicolonEntityValue, 1 },
    { hellipSemicolonEntityName, 7, hellipSemicolonEntityValue, 1 },
    { herconSemicolonEntityName, 7, herconSemicolonEntityValue, 1 },
    { hfrSemicolonEntityName, 4, hfrSemicolonEntityValue, 1 },
    { hksearowSemicolonEntityName, 9, hksearowSemicolonEntityValue, 1 },
    { hkswarowSemicolonEntityName, 9, hkswarowSemicolonEntityValue, 1 },
    { hoarrSemicolonEntityName, 6, hoarrSemicolonEntityValue, 1 },
    { homthtSemicolonEntityName, 7, homthtSemicolonEntityValue, 1 },
    { hookleftarrowSemicolonEntityName, 14, hookleftarrowSemicolonEntityValue, 1 },
    { hookrightarrowSemicolonEntityName, 15, hookrightarrowSemicolonEntityValue, 1 },
    { hopfSemicolonEntityName, 5, hopfSemicolonEntityValue, 1 },
    { horbarSemicolonEntityName, 7, horbarSemicolonEntityValue, 1 },
    { hscrSemicolonEntityName, 5, hscrSemicolonEntityValue, 1 },
    { hslashSemicolonEntityName, 7, hslashSemicolonEntityValue, 1 },
    { hstrokSemicolonEntityName, 7, hstrokSemicolonEntityValue, 1 },
    { hybullSemicolonEntityName, 7, hybullSemicolonEntityValue, 1 },
    { hyphenSemicolonEntityName, 7, hyphenSemicolonEntityValue, 1 },
    { iacuteEntityName, 6, iacuteEntityValue, 1 },
    { iacuteSemicolonEntityName, 7, iacuteSemicolonEntityValue, 1 },
    { icSemicolonEntityName, 3, icSemicolonEntityValue, 1 },
    { icircEntityName, 5, icircEntityValue, 1 },
    { icircSemicolonEntityName, 6, icircSemicolonEntityValue, 1 },
    { icySemicolonEntityName, 4, icySemicolonEntityValue, 1 },
    { iecySemicolonEntityName, 5, iecySemicolonEntityValue, 1 },
    { iexclEntityName, 5, iexclEntityValue, 1 },
    { iexclSemicolonEntityName, 6, iexclSemicolonEntityValue, 1 },
    { iffSemicolonEntityName, 4, iffSemicolonEntityValue, 1 },
    { ifrSemicolonEntityName, 4, ifrSemicolonEntityValue, 1 },
    { igraveEntityName, 6, igraveEntityValue, 1 },
    { igraveSemicolonEntityName, 7, igraveSemicolonEntityValue, 1 },
    { iiSemicolonEntityName, 3, iiSemicolonEntityValue, 1 },
    { iiiintSemicolonEntityName, 7, iiiintSemicolonEntityValue, 1 },
    { iiintSemicolonEntityName, 6, iiintSemicolonEntityValue, 1 },
    { iinfinSemicolonEntityName, 7, iinfinSemicolonEntityValue, 1 },
    { iiotaSemicolonEntityName, 6, iiotaSemicolonEntityValue, 1 },
    { ijligSemicolonEntityName, 6, ijligSemicolonEntityValue, 1 },
    { imacrSemicolonEntityName, 6, imacrSemicolonEntityValue, 1 },
    { imageSemicolonEntityName, 6, imageSemicolonEntityValue, 1 },
    { imaglineSemicolonEntityName, 9, imaglineSemicolonEntityValue, 1 },
    { imagpartSemicolonEntityName, 9, imagpartSemicolonEntityValue, 1 },
    { imathSemicolonEntityName, 6, imathSemicolonEntityValue, 1 },
    { imofSemicolonEntityName, 5, imofSemicolonEntityValue, 1 },
    { impedSemicolonEntityName, 6, impedSemicolonEntityValue, 1 },
    { inSemicolonEntityName, 3, inSemicolonEntityValue, 1 },
    { incareSemicolonEntityName, 7, incareSemicolonEntityValue, 1 },
    { infinSemicolonEntityName, 6, infinSemicolonEntityValue, 1 },
    { infintieSemicolonEntityName, 9, infintieSemicolonEntityValue, 1 },
    { inodotSemicolonEntityName, 7, inodotSemicolonEntityValue, 1 },
    { intSemicolonEntityName, 4, intSemicolonEntityValue, 1 },
    { intcalSemicolonEntityName, 7, intcalSemicolonEntityValue, 1 },
    { integersSemicolonEntityName, 9, integersSemicolonEntityValue, 1 },
    { intercalSemicolonEntityName, 9, intercalSemicolonEntityValue, 1 },
    { intlarhkSemicolonEntityName, 9, intlarhkSemicolonEntityValue, 1 },
    { intprodSemicolonEntityName, 8, intprodSemicolonEntityValue, 1 },
    { iocySemicolonEntityName, 5, iocySemicolonEntityValue, 1 },
    { iogonSemicolonEntityName, 6, iogonSemicolonEntityValue, 1 },
    { iopfSemicolonEntityName, 5, iopfSemicolonEntityValue, 1 },
    { iotaSemicolonEntityName, 5, iotaSemicolonEntityValue, 1 },
    { iprodSemicolonEntityName, 6, iprodSemicolonEntityValue, 1 },
    { iquestEntityName, 6, iquestEntityValue, 1 },
    { iquestSemicolonEntityName, 7, iquestSemicolonEntityValue, 1 },
    { iscrSemicolonEntityName, 5, iscrSemicolonEntityValue, 1 },
    { isinSemicolonEntityName, 5, isinSemicolonEntityValue, 1 },
    { isinESemicolonEntityName, 6, isinESemicolonEntityValue, 1 },
    { isindotSemicolonEntityName, 8, isindotSemicolonEntityValue, 1 },
    { isinsSemicolonEntityName, 6, isinsSemicolonEntityValue, 1 },
    { isinsvSemicolonEntityName, 7, isinsvSemicolonEntityValue, 1 },
    { isinvSemicolonEntityName, 6, isinvSemicolonEntityValue, 1 },
    { itSemicolonEntityName, 3, itSemicolonEntityValue, 1 },
    { itildeSemicolonEntityName, 7, itildeSemicolonEntityValue, 1 },
    { iukcySemicolonEntityName, 6, iukcySemicolonEntityValue, 1 },
    { iumlEntityName, 4, iumlEntityValue, 1 },
    { iumlSemicolonEntityName, 5, iumlSemicolonEntityValue, 1 },
    { jcircSemicolonEntityName, 6, jcircSemicolonEntityValue, 1 },
    { jcySemicolonEntityName, 4, jcySemicolonEntityValue, 1 },
    { jfrSemicolonEntityName, 4, jfrSemicolonEntityValue, 1 },
    { jmathSemicolonEntityName, 6, jmathSemicolonEntityValue, 1 },
    { jopfSemicolonEntityName, 5, jopfSemicolonEntityValue, 1 },
    { jscrSemicolonEntityName, 5, jscrSemicolonEntityValue, 1 },
    { jsercySemicolonEntityName, 7, jsercySemicolonEntityValue, 1 },
    { jukcySemicolonEntityName, 6, jukcySemicolonEntityValue, 1 },
    { kappaSemicolonEntityName, 6, kappaSemicolonEntityValue, 1 },
    { kappavSemicolonEntityName, 7, kappavSemicolonEntityValue, 1 },
    { kcedilSemicolonEntityName, 7, kcedilSemicolonEntityValue, 1 },
    { kcySemicolonEntityName, 4, kcySemicolonEntityValue, 1 },
    { kfrSemicolonEntityName, 4, kfrSemicolonEntityValue, 1 },
    { kgreenSemicolonEntityName, 7, kgreenSemicolonEntityValue, 1 },
    { khcySemicolonEntityName, 5, khcySemicolonEntityValue, 1 },
    { kjcySemicolonEntityName, 5, kjcySemicolonEntityValue, 1 },
    { kopfSemicolonEntityName, 5, kopfSemicolonEntityValue, 1 },
    { kscrSemicolonEntityName, 5, kscrSemicolonEntityValue, 1 },
    { lAarrSemicolonEntityName, 6, lAarrSemicolonEntityValue, 1 },
    { lArrSemicolonEntityName, 5, lArrSemicolonEntityValue, 1 },
    { lAtailSemicolonEntityName, 7, lAtailSemicolonEntityValue, 1 },
    { lBarrSemicolonEntityName, 6, lBarrSemicolonEntityValue, 1 },
    { lESemicolonEntityName, 3, lESemicolonEntityValue, 1 },
    { lEgSemicolonEntityName, 4, lEgSemicolonEntityValue, 1 },
    { lHarSemicolonEntityName, 5, lHarSemicolonEntityValue, 1 },
    { lacuteSemicolonEntityName, 7, lacuteSemicolonEntityValue, 1 },
    { laemptyvSemicolonEntityName, 9, laemptyvSemicolonEntityValue, 1 },
    { lagranSemicolonEntityName, 7, lagranSemicolonEntityValue, 1 },
    { lambdaSemicolonEntityName, 7, lambdaSemicolonEntityValue, 1 },
    { langSemicolonEntityName, 5, langSemicolonEntityValue, 1 },
    { langdSemicolonEntityName, 6, langdSemicolonEntityValue, 1 },
    { langleSemicolonEntityName, 7, langleSemicolonEntityValue, 1 },
    { lapSemicolonEntityName, 4, lapSemicolonEntityValue, 1 },
    { laquoEntityName, 5, laquoEntityValue, 1 },
    { laquoSemicolonEntityName, 6, laquoSemicolonEntityValue, 1 },
    { larrSemicolonEntityName, 5, larrSemicolonEntityValue, 1 },
    { larrbSemicolonEntityName, 6, larrbSemicolonEntityValue, 1 },
    { larrbfsSemicolonEntityName, 8, larrbfsSemicolonEntityValue, 1 },
    { larrfsSemicolonEntityName, 7, larrfsSemicolonEntityValue, 1 },
    { larrhkSemicolonEntityName, 7, larrhkSemicolonEntityValue, 1 },
    { larrlpSemicolonEntityName, 7, larrlpSemicolonEntityValue, 1 },
    { larrplSemicolonEntityName, 7, larrplSemicolonEntityValue, 1 },
    { larrsimSemicolonEntityName, 8, larrsimSemicolonEntityValue, 1 },
    { larrtlSemicolonEntityName, 7, larrtlSemicolonEntityValue, 1 },
    { latSemicolonEntityName, 4, latSemicolonEntityValue, 1 },
    { latailSemicolonEntityName, 7, latailSemicolonEntityValue, 1 },
    { lateSemicolonEntityName, 5, lateSemicolonEntityValue, 1 },
    { latesSemicolonEntityName, 6, latesSemicolonEntityValue, 2 },
    { lbarrSemicolonEntityName, 6, lbarrSemicolonEntityValue, 1 },
    { lbbrkSemicolonEntityName, 6, lbbrkSemicolonEntityValue, 1 },
    { lbraceSemicolonEntityName, 7, lbraceSemicolonEntityValue, 1 },
    { lbrackSemicolonEntityName, 7, lbrackSemicolonEntityValue, 1 },
    { lbrkeSemicolonEntityName, 6, lbrkeSemicolonEntityValue, 1 },
    { lbrksldSemicolonEntityName, 8, lbrksldSemicolonEntityValue, 1 },
    { lbrksluSemicolonEntityName, 8, lbrksluSemicolonEntityValue, 1 },
    { lcaronSemicolonEntityName, 7, lcaronSemicolonEntityValue, 1 },
    { lcedilSemicolonEntityName, 7, lcedilSemicolonEntityValue, 1 },
    { lceilSemicolonEntityName, 6, lceilSemicolonEntityValue, 1 },
    { lcubSemicolonEntityName, 5, lcubSemicolonEntityValue, 1 },
    { lcySemicolonEntityName, 4, lcySemicolonEntityValue, 1 },
    { ldcaSemicolonEntityName, 5, ldcaSemicolonEntityValue, 1 },
    { ldquoSemicolonEntityName, 6, ldquoSemicolonEntityValue, 1 },
    { ldquorSemicolonEntityName, 7, ldquorSemicolonEntityValue, 1 },
    { ldrdharSemicolonEntityName, 8, ldrdharSemicolonEntityValue, 1 },
    { ldrusharSemicolonEntityName, 9, ldrusharSemicolonEntityValue, 1 },
    { ldshSemicolonEntityName, 5, ldshSemicolonEntityValue, 1 },
    { leSemicolonEntityName, 3, leSemicolonEntityValue, 1 },
    { leftarrowSemicolonEntityName, 10, leftarrowSemicolonEntityValue, 1 },
    { leftarrowtailSemicolonEntityName, 14, leftarrowtailSemicolonEntityValue, 1 },
    { leftharpoondownSemicolonEntityName, 16, leftharpoondownSemicolonEntityValue, 1 },
    { leftharpoonupSemicolonEntityName, 14, leftharpoonupSemicolonEntityValue, 1 },
    { leftleftarrowsSemicolonEntityName, 15, leftleftarrowsSemicolonEntityValue, 1 },
    { leftrightarrowSemicolonEntityName, 15, leftrightarrowSemicolonEntityValue, 1 },
    { leftrightarrowsSemicolonEntityName, 16, leftrightarrowsSemicolonEntityValue, 1 },
    { leftrightharpoonsSemicolonEntityName, 18, leftrightharpoonsSemicolonEntityValue, 1 },
    { leftrightsquigarrowSemicolonEntityName, 20, leftrightsquigarrowSemicolonEntityValue, 1 },
    { leftthreetimesSemicolonEntityName, 15, leftthreetimesSemicolonEntityValue, 1 },
    { legSemicolonEntityName, 4, legSemicolonEntityValue, 1 },
    { leqSemicolonEntityName, 4, leqSemicolonEntityValue, 1 },
    { leqqSemicolonEntityName, 5, leqqSemicolonEntityValue, 1 },
    { leqslantSemicolonEntityName, 9, leqslantSemicolonEntityValue, 1 },
    { lesSemicolonEntityName, 4, lesSemicolonEntityValue, 1 },
    { lesccSemicolonEntityName, 6, lesccSemicolonEntityValue, 1 },
    { lesdotSemicolonEntityName, 7, lesdotSemicolonEntityValue, 1 },
    { lesdotoSemicolonEntityName, 8, lesdotoSemicolonEntityValue, 1 },
    { lesdotorSemicolonEntityName, 9, lesdotorSemicolonEntityValue, 1 },
    { lesgSemicolonEntityName, 5, lesgSemicolonEntityValue, 2 },
    { lesgesSemicolonEntityName, 7, lesgesSemicolonEntityValue, 1 },
    { lessapproxSemicolonEntityName, 11, lessapproxSemicolonEntityValue, 1 },
    { lessdotSemicolonEntityName, 8, lessdotSemicolonEntityValue, 1 },
    { lesseqgtrSemicolonEntityName, 10, lesseqgtrSemicolonEntityValue, 1 },
    { lesseqqgtrSemicolonEntityName, 11, lesseqqgtrSemicolonEntityValue, 1 },
    { lessgtrSemicolonEntityName, 8, lessgtrSemicolonEntityValue, 1 },
    { lesssimSemicolonEntityName, 8, lesssimSemicolonEntityValue, 1 },
    { lfishtSemicolonEntityName, 7, lfishtSemicolonEntityValue, 1 },
    { lfloorSemicolonEntityName, 7, lfloorSemicolonEntityValue, 1 },
    { lfrSemicolonEntityName, 4, lfrSemicolonEntityValue, 1 },
    { lgSemicolonEntityName, 3, lgSemicolonEntityValue, 1 },
    { lgESemicolonEntityName, 4, lgESemicolonEntityValue, 1 },
    { lhardSemicolonEntityName, 6, lhardSemicolonEntityValue, 1 },
    { lharuSemicolonEntityName, 6, lharuSemicolonEntityValue, 1 },
    { lharulSemicolonEntityName, 7, lharulSemicolonEntityValue, 1 },
    { lhblkSemicolonEntityName, 6, lhblkSemicolonEntityValue, 1 },
    { ljcySemicolonEntityName, 5, ljcySemicolonEntityValue, 1 },
    { llSemicolonEntityName, 3, llSemicolonEntityValue, 1 },
    { llarrSemicolonEntityName, 6, llarrSemicolonEntityValue, 1 },
    { llcornerSemicolonEntityName, 9, llcornerSemicolonEntityValue, 1 },
    { llhardSemicolonEntityName, 7, llhardSemicolonEntityValue, 1 },
    { lltriSemicolonEntityName, 6, lltriSemicolonEntityValue, 1 },
    { lmidotSemicolonEntityName, 7, lmidotSemicolonEntityValue, 1 },
    { lmoustSemicolonEntityName, 7, lmoustSemicolonEntityValue, 1 },
    { lmoustacheSemicolonEntityName, 11, lmoustacheSemicolonEntityValue, 1 },
    { lnESemicolonEntityName, 4, lnESemicolonEntityValue, 1 },
    { lnapSemicolonEntityName, 5, lnapSemicolonEntityValue, 1 },
    { lnapproxSemicolonEntityName, 9, lnapproxSemicolonEntityValue, 1 },
    { lneSemicolonEntityName, 4, lneSemicolonEntityValue, 1 },
    { lneqSemicolonEntityName, 5, lneqSemicolonEntityValue, 1 },
    { lneqqSemicolonEntityName, 6, lneqqSemicolonEntityValue, 1 },
    { lnsimSemicolonEntityName, 6, lnsimSemicolonEntityValue, 1 },
    { loangSemicolonEntityName, 6, loangSemicolonEntityValue, 1 },
    { loarrSemicolonEntityName, 6, loarrSemicolonEntityValue, 1 },
    { lobrkSemicolonEntityName, 6, lobrkSemicolonEntityValue, 1 },
    { longleftarrowSemicolonEntityName, 14, longleftarrowSemicolonEntityValue, 1 },
    { longleftrightarrowSemicolonEntityName, 19, longleftrightarrowSemicolonEntityValue, 1 },
    { longmapstoSemicolonEntityName, 11, longmapstoSemicolonEntityValue, 1 },
    { longrightarrowSemicolonEntityName, 15, longrightarrowSemicolonEntityValue, 1 },
    { looparrowleftSemicolonEntityName, 14, looparrowleftSemicolonEntityValue, 1 },
    { looparrowrightSemicolonEntityName, 15, looparrowrightSemicolonEntityValue, 1 },
    { loparSemicolonEntityName, 6, loparSemicolonEntityValue, 1 },
    { lopfSemicolonEntityName, 5, lopfSemicolonEntityValue, 1 },
    { loplusSemicolonEntityName, 7, loplusSemicolonEntityValue, 1 },
    { lotimesSemicolonEntityName, 8, lotimesSemicolonEntityValue, 1 },
    { lowastSemicolonEntityName, 7, lowastSemicolonEntityValue, 1 },
    { lowbarSemicolonEntityName, 7, lowbarSemicolonEntityValue, 1 },
    { lozSemicolonEntityName, 4, lozSemicolonEntityValue, 1 },
    { lozengeSemicolonEntityName, 8, lozengeSemicolonEntityValue, 1 },
    { lozfSemicolonEntityName, 5, lozfSemicolonEntityValue, 1 },
    { lparSemicolonEntityName, 5, lparSemicolonEntityValue, 1 },
    { lparltSemicolonEntityName, 7, lparltSemicolonEntityValue, 1 },
    { lrarrSemicolonEntityName, 6, lrarrSemicolonEntityValue, 1 },
    { lrcornerSemicolonEntityName, 9, lrcornerSemicolonEntityValue, 1 },
    { lrharSemicolonEntityName, 6, lrharSemicolonEntityValue, 1 },
    { lrhardSemicolonEntityName, 7, lrhardSemicolonEntityValue, 1 },
    { lrmSemicolonEntityName, 4, lrmSemicolonEntityValue, 1 },
    { lrtriSemicolonEntityName, 6, lrtriSemicolonEntityValue, 1 },
    { lsaquoSemicolonEntityName, 7, lsaquoSemicolonEntityValue, 1 },
    { lscrSemicolonEntityName, 5, lscrSemicolonEntityValue, 1 },
    { lshSemicolonEntityName, 4, lshSemicolonEntityValue, 1 },
    { lsimSemicolonEntityName, 5, lsimSemicolonEntityValue, 1 },
    { lsimeSemicolonEntityName, 6, lsimeSemicolonEntityValue, 1 },
    { lsimgSemicolonEntityName, 6, lsimgSemicolonEntityValue, 1 },
    { lsqbSemicolonEntityName, 5, lsqbSemicolonEntityValue, 1 },
    { lsquoSemicolonEntityName, 6, lsquoSemicolonEntityValue, 1 },
    { lsquorSemicolonEntityName, 7, lsquorSemicolonEntityValue, 1 },
    { lstrokSemicolonEntityName, 7, lstrokSemicolonEntityValue, 1 },
    { ltEntityName, 2, ltEntityValue, 1 },
    { ltSemicolonEntityName, 3, ltSemicolonEntityValue, 1 },
    { ltccSemicolonEntityName, 5, ltccSemicolonEntityValue, 1 },
    { ltcirSemicolonEntityName, 6, ltcirSemicolonEntityValue, 1 },
    { ltdotSemicolonEntityName, 6, ltdotSemicolonEntityValue, 1 },
    { lthreeSemicolonEntityName, 7, lthreeSemicolonEntityValue, 1 },
    { ltimesSemicolonEntityName, 7, ltimesSemicolonEntityValue, 1 },
    { ltlarrSemicolonEntityName, 7, ltlarrSemicolonEntityValue, 1 },
    { ltquestSemicolonEntityName, 8, ltquestSemicolonEntityValue, 1 },
    { ltrParSemicolonEntityName, 7, ltrParSemicolonEntityValue, 1 },
    { ltriSemicolonEntityName, 5, ltriSemicolonEntityValue, 1 },
    { ltrieSemicolonEntityName, 6, ltrieSemicolonEntityValue, 1 },
    { ltrifSemicolonEntityName, 6, ltrifSemicolonEntityValue, 1 },
    { lurdsharSemicolonEntityName, 9, lurdsharSemicolonEntityValue, 1 },
    { luruharSemicolonEntityName, 8, luruharSemicolonEntityValue, 1 },
    { lvertneqqSemicolonEntityName, 10, lvertneqqSemicolonEntityValue, 2 },
    { lvnESemicolonEntityName, 5, lvnESemicolonEntityValue, 2 },
    { mDDotSemicolonEntityName, 6, mDDotSemicolonEntityValue, 1 },
    { macrEntityName, 4, macrEntityValue, 1 },
    { macrSemicolonEntityName, 5, macrSemicolonEntityValue, 1 },
    { maleSemicolonEntityName, 5, maleSemicolonEntityValue, 1 },
    { maltSemicolonEntityName, 5, maltSemicolonEntityValue, 1 },
    { malteseSemicolonEntityName, 8, malteseSemicolonEntityValue, 1 },
    { mapSemicolonEntityName, 4, mapSemicolonEntityValue, 1 },
    { mapstoSemicolonEntityName, 7, mapstoSemicolonEntityValue, 1 },
    { mapstodownSemicolonEntityName, 11, mapstodownSemicolonEntityValue, 1 },
    { mapstoleftSemicolonEntityName, 11, mapstoleftSemicolonEntityValue, 1 },
    { mapstoupSemicolonEntityName, 9, mapstoupSemicolonEntityValue, 1 },
    { markerSemicolonEntityName, 7, markerSemicolonEntityValue, 1 },
    { mcommaSemicolonEntityName, 7, mcommaSemicolonEntityValue, 1 },
    { mcySemicolonEntityName, 4, mcySemicolonEntityValue, 1 },
    { mdashSemicolonEntityName, 6, mdashSemicolonEntityValue, 1 },
    { measuredangleSemicolonEntityName, 14, measuredangleSemicolonEntityValue, 1 },
    { mfrSemicolonEntityName, 4, mfrSemicolonEntityValue, 1 },
    { mhoSemicolonEntityName, 4, mhoSemicolonEntityValue, 1 },
    { microEntityName, 5, microEntityValue, 1 },
    { microSemicolonEntityName, 6, microSemicolonEntityValue, 1 },
    { midSemicolonEntityName, 4, midSemicolonEntityValue, 1 },
    { midastSemicolonEntityName, 7, midastSemicolonEntityValue, 1 },
    { midcirSemicolonEntityName, 7, midcirSemicolonEntityValue, 1 },
    { middotEntityName, 6, middotEntityValue, 1 },
    { middotSemicolonEntityName, 7, middotSemicolonEntityValue, 1 },
    { minusSemicolonEntityName, 6, minusSemicolonEntityValue, 1 },
    { minusbSemicolonEntityName, 7, minusbSemicolonEntityValue, 1 },
    { minusdSemicolonEntityName, 7, minusdSemicolonEntityValue, 1 },
    { minusduSemicolonEntityName, 8, minusduSemicolonEntityValue, 1 },
    { mlcpSemicolonEntityName, 5, mlcpSemicolonEntityValue, 1 },
    { mldrSemicolonEntityName, 5, mldrSemicolonEntityValue, 1 },
    { mnplusSemicolonEntityName, 7, mnplusSemicolonEntityValue, 1 },
    { modelsSemicolonEntityName, 7, modelsSemicolonEntityValue, 1 },
    { mopfSemicolonEntityName, 5, mopfSemicolonEntityValue, 1 },
    { mpSemicolonEntityName, 3, mpSemicolonEntityValue, 1 },
    { mscrSemicolonEntityName, 5, mscrSemicolonEntityValue, 1 },
    { mstposSemicolonEntityName, 7, mstposSemicolonEntityValue, 1 },
    { muSemicolonEntityName, 3, muSemicolonEntityValue, 1 },
    { multimapSemicolonEntityName, 9, multimapSemicolonEntityValue, 1 },
    { mumapSemicolonEntityName, 6, mumapSemicolonEntityValue, 1 },
    { nGgSemicolonEntityName, 4, nGgSemicolonEntityValue, 2 },
    { nGtSemicolonEntityName, 4, nGtSemicolonEntityValue, 2 },
    { nGtvSemicolonEntityName, 5, nGtvSemicolonEntityValue, 2 },
    { nLeftarrowSemicolonEntityName, 11, nLeftarrowSemicolonEntityValue, 1 },
    { nLeftrightarrowSemicolonEntityName, 16, nLeftrightarrowSemicolonEntityValue, 1 },
    { nLlSemicolonEntityName, 4, nLlSemicolonEntityValue, 2 },
    { nLtSemicolonEntityName, 4, nLtSemicolonEntityValue, 2 },
    { nLtvSemicolonEntityName, 5, nLtvSemicolonEntityValue, 2 },
    { nRightarrowSemicolonEntityName, 12, nRightarrowSemicolonEntityValue, 1 },
    { nVDashSemicolonEntityName, 7, nVDashSemicolonEntityValue, 1 },
    { nVdashSemicolonEntityName, 7, nVdashSemicolonEntityValue, 1 },
    { nablaSemicolonEntityName, 6, nablaSemicolonEntityValue, 1 },
    { nacuteSemicolonEntityName, 7, nacuteSemicolonEntityValue, 1 },
    { nangSemicolonEntityName, 5, nangSemicolonEntityValue, 2 },
    { napSemicolonEntityName, 4, napSemicolonEntityValue, 1 },
    { napESemicolonEntityName, 5, napESemicolonEntityValue, 2 },
    { napidSemicolonEntityName, 6, napidSemicolonEntityValue, 2 },
    { naposSemicolonEntityName, 6, naposSemicolonEntityValue, 1 },
    { napproxSemicolonEntityName, 8, napproxSemicolonEntityValue, 1 },
    { naturSemicolonEntityName, 6, naturSemicolonEntityValue, 1 },
    { naturalSemicolonEntityName, 8, naturalSemicolonEntityValue, 1 },
    { naturalsSemicolonEntityName, 9, naturalsSemicolonEntityValue, 1 },
    { nbspEntityName, 4, nbspEntityValue, 1 },
    { nbspSemicolonEntityName, 5, nbspSemicolonEntityValue, 1 },
    { nbumpSemicolonEntityName, 6, nbumpSemicolonEntityValue, 2 },
    { nbumpeSemicolonEntityName, 7, nbumpeSemicolonEntityValue, 2 },
    { ncapSemicolonEntityName, 5, ncapSemicolonEntityValue, 1 },
    { ncaronSemicolonEntityName, 7, ncaronSemicolonEntityValue, 1 },
    { ncedilSemicolonEntityName, 7, ncedilSemicolonEntityValue, 1 },
    { ncongSemicolonEntityName, 6, ncongSemicolonEntityValue, 1 },
    { ncongdotSemicolonEntityName, 9, ncongdotSemicolonEntityValue, 2 },
    { ncupSemicolonEntityName, 5, ncupSemicolonEntityValue, 1 },
    { ncySemicolonEntityName, 4, ncySemicolonEntityValue, 1 },
    { ndashSemicolonEntityName, 6, ndashSemicolonEntityValue, 1 },
    { neSemicolonEntityName, 3, neSemicolonEntityValue, 1 },
    { neArrSemicolonEntityName, 6, neArrSemicolonEntityValue, 1 },
    { nearhkSemicolonEntityName, 7, nearhkSemicolonEntityValue, 1 },
    { nearrSemicolonEntityName, 6, nearrSemicolonEntityValue, 1 },
    { nearrowSemicolonEntityName, 8, nearrowSemicolonEntityValue, 1 },
    { nedotSemicolonEntityName, 6, nedotSemicolonEntityValue, 2 },
    { nequivSemicolonEntityName, 7, nequivSemicolonEntityValue, 1 },
    { nesearSemicolonEntityName, 7, nesearSemicolonEntityValue, 1 },
    { nesimSemicolonEntityName, 6, nesimSemicolonEntityValue, 2 },
    { nexistSemicolonEntityName, 7, nexistSemicolonEntityValue, 1 },
    { nexistsSemicolonEntityName, 8, nexistsSemicolonEntityValue, 1 },
    { nfrSemicolonEntityName, 4, nfrSemicolonEntityValue, 1 },
    { ngESemicolonEntityName, 4, ngESemicolonEntityValue, 2 },
    { ngeSemicolonEntityName, 4, ngeSemicolonEntityValue, 1 },
    { ngeqSemicolonEntityName, 5, ngeqSemicolonEntityValue, 1 },
    { ngeqqSemicolonEntityName, 6, ngeqqSemicolonEntityValue, 2 },
    { ngeqslantSemicolonEntityName, 10, ngeqslantSemicolonEntityValue, 2 },
    { ngesSemicolonEntityName, 5, ngesSemicolonEntityValue, 2 },
    { ngsimSemicolonEntityName, 6, ngsimSemicolonEntityValue, 1 },
    { ngtSemicolonEntityName, 4, ngtSemicolonEntityValue, 1 },
    { ngtrSemicolonEntityName, 5, ngtrSemicolonEntityValue, 1 },
    { nhArrSemicolonEntityName, 6, nhArrSemicolonEntityValue, 1 },
    { nharrSemicolonEntityName, 6, nharrSemicolonEntityValue, 1 },
    { nhparSemicolonEntityName, 6, nhparSemicolonEntityValue, 1 },
    { niSemicolonEntityName, 3, niSemicolonEntityValue, 1 },
    { nisSemicolonEntityName, 4, nisSemicolonEntityValue, 1 },
    { nisdSemicolonEntityName, 5, nisdSemicolonEntityValue, 1 },
    { nivSemicolonEntityName, 4, nivSemicolonEntityValue, 1 },
    { njcySemicolonEntityName, 5, njcySemicolonEntityValue, 1 },
    { nlArrSemicolonEntityName, 6, nlArrSemicolonEntityValue, 1 },
    { nlESemicolonEntityName, 4, nlESemicolonEntityValue, 2 },
    { nlarrSemicolonEntityName, 6, nlarrSemicolonEntityValue, 1 },
    { nldrSemicolonEntityName, 5, nldrSemicolonEntityValue, 1 },
    { nleSemicolonEntityName, 4, nleSemicolonEntityValue, 1 },
    { nleftarrowSemicolonEntityName, 11, nleftarrowSemicolonEntityValue, 1 },
    { nleftrightarrowSemicolonEntityName, 16, nleftrightarrowSemicolonEntityValue, 1 },
    { nleqSemicolonEntityName, 5, nleqSemicolonEntityValue, 1 },
    { nleqqSemicolonEntityName, 6, nleqqSemicolonEntityValue, 2 },
    { nleqslantSemicolonEntityName, 10, nleqslantSemicolonEntityValue, 2 },
    { nlesSemicolonEntityName, 5, nlesSemicolonEntityValue, 2 },
    { nlessSemicolonEntityName, 6, nlessSemicolonEntityValue, 1 },
    { nlsimSemicolonEntityName, 6, nlsimSemicolonEntityValue, 1 },
    { nltSemicolonEntityName, 4, nltSemicolonEntityValue, 1 },
    { nltriSemicolonEntityName, 6, nltriSemicolonEntityValue, 1 },
    { nltrieSemicolonEntityName, 7, nltrieSemicolonEntityValue, 1 },
    { nmidSemicolonEntityName, 5, nmidSemicolonEntityValue, 1 },
    { nopfSemicolonEntityName, 5, nopfSemicolonEntityValue, 1 },
    { notEntityName, 3, notEntityValue, 1 },
    { notSemicolonEntityName, 4, notSemicolonEntityValue, 1 },
    { notinSemicolonEntityName, 6, notinSemicolonEntityValue, 1 },
    { notinESemicolonEntityName, 7, notinESemicolonEntityValue, 2 },
    { notindotSemicolonEntityName, 9, notindotSemicolonEntityValue, 2 },
    { notinvaSemicolonEntityName, 8, notinvaSemicolonEntityValue, 1 },
    { notinvbSemicolonEntityName, 8, notinvbSemicolonEntityValue, 1 },
    { notinvcSemicolonEntityName, 8, notinvcSemicolonEntityValue, 1 },
    { notniSemicolonEntityName, 6, notniSemicolonEntityValue, 1 },
    { notnivaSemicolonEntityName, 8, notnivaSemicolonEntityValue, 1 },
    { notnivbSemicolonEntityName, 8, notnivbSemicolonEntityValue, 1 },
    { notnivcSemicolonEntityName, 8, notnivcSemicolonEntityValue, 1 },
    { nparSemicolonEntityName, 5, nparSemicolonEntityValue, 1 },
    { nparallelSemicolonEntityName, 10, nparallelSemicolonEntityValue, 1 },
    { nparslSemicolonEntityName, 7, nparslSemicolonEntityValue, 2 },
    { npartSemicolonEntityName, 6, npartSemicolonEntityValue, 2 },
    { npolintSemicolonEntityName, 8, npolintSemicolonEntityValue, 1 },
    { nprSemicolonEntityName, 4, nprSemicolonEntityValue, 1 },
    { nprcueSemicolonEntityName, 7, nprcueSemicolonEntityValue, 1 },
    { npreSemicolonEntityName, 5, npreSemicolonEntityValue, 2 },
    { nprecSemicolonEntityName, 6, nprecSemicolonEntityValue, 1 },
    { npreceqSemicolonEntityName, 8, npreceqSemicolonEntityValue, 2 },
    { nrArrSemicolonEntityName, 6, nrArrSemicolonEntityValue, 1 },
    { nrarrSemicolonEntityName, 6, nrarrSemicolonEntityValue, 1 },
    { nrarrcSemicolonEntityName, 7, nrarrcSemicolonEntityValue, 2 },
    { nrarrwSemicolonEntityName, 7, nrarrwSemicolonEntityValue, 2 },
    { nrightarrowSemicolonEntityName, 12, nrightarrowSemicolonEntityValue, 1 },
    { nrtriSemicolonEntityName, 6, nrtriSemicolonEntityValue, 1 },
    { nrtrieSemicolonEntityName, 7, nrtrieSemicolonEntityValue, 1 },
    { nscSemicolonEntityName, 4, nscSemicolonEntityValue, 1 },
    { nsccueSemicolonEntityName, 7, nsccueSemicolonEntityValue, 1 },
    { nsceSemicolonEntityName, 5, nsceSemicolonEntityValue, 2 },
    { nscrSemicolonEntityName, 5, nscrSemicolonEntityValue, 1 },
    { nshortmidSemicolonEntityName, 10, nshortmidSemicolonEntityValue, 1 },
    { nshortparallelSemicolonEntityName, 15, nshortparallelSemicolonEntityValue, 1 },
    { nsimSemicolonEntityName, 5, nsimSemicolonEntityValue, 1 },
    { nsimeSemicolonEntityName, 6, nsimeSemicolonEntityValue, 1 },
    { nsimeqSemicolonEntityName, 7, nsimeqSemicolonEntityValue, 1 },
    { nsmidSemicolonEntityName, 6, nsmidSemicolonEntityValue, 1 },
    { nsparSemicolonEntityName, 6, nsparSemicolonEntityValue, 1 },
    { nsqsubeSemicolonEntityName, 8, nsqsubeSemicolonEntityValue, 1 },
    { nsqsupeSemicolonEntityName, 8, nsqsupeSemicolonEntityValue, 1 },
    { nsubSemicolonEntityName, 5, nsubSemicolonEntityValue, 1 },
    { nsubESemicolonEntityName, 6, nsubESemicolonEntityValue, 2 },
    { nsubeSemicolonEntityName, 6, nsubeSemicolonEntityValue, 1 },
    { nsubsetSemicolonEntityName, 8, nsubsetSemicolonEntityValue, 2 },
    { nsubseteqSemicolonEntityName, 10, nsubseteqSemicolonEntityValue, 1 },
    { nsubseteqqSemicolonEntityName, 11, nsubseteqqSemicolonEntityValue, 2 },
    { nsuccSemicolonEntityName, 6, nsuccSemicolonEntityValue, 1 },
    { nsucceqSemicolonEntityName, 8, nsucceqSemicolonEntityValue, 2 },
    { nsupSemicolonEntityName, 5, nsupSemicolonEntityValue, 1 },
    { nsupESemicolonEntityName, 6, nsupESemicolonEntityValue, 2 },
    { nsupeSemicolonEntityName, 6, nsupeSemicolonEntityValue, 1 },
    { nsupsetSemicolonEntityName, 8, nsupsetSemicolonEntityValue, 2 },
    { nsupseteqSemicolonEntityName, 10, nsupseteqSemicolonEntityValue, 1 },
    { nsupseteqqSemicolonEntityName, 11, nsupseteqqSemicolonEntityValue, 2 },
    { ntglSemicolonEntityName, 5, ntglSemicolonEntityValue, 1 },
    { ntildeEntityName, 6, ntildeEntityValue, 1 },
    { ntildeSemicolonEntityName, 7, ntildeSemicolonEntityValue, 1 },
    { ntlgSemicolonEntityName, 5, ntlgSemicolonEntityValue, 1 },
    { ntriangleleftSemicolonEntityName, 14, ntriangleleftSemicolonEntityValue, 1 },
    { ntrianglelefteqSemicolonEntityName, 16, ntrianglelefteqSemicolonEntityValue, 1 },
    { ntrianglerightSemicolonEntityName, 15, ntrianglerightSemicolonEntityValue, 1 },
    { ntrianglerighteqSemicolonEntityName, 17, ntrianglerighteqSemicolonEntityValue, 1 },
    { nuSemicolonEntityName, 3, nuSemicolonEntityValue, 1 },
    { numSemicolonEntityName, 4, numSemicolonEntityValue, 1 },
    { numeroSemicolonEntityName, 7, numeroSemicolonEntityValue, 1 },
    { numspSemicolonEntityName, 6, numspSemicolonEntityValue, 1 },
    { nvDashSemicolonEntityName, 7, nvDashSemicolonEntityValue, 1 },
    { nvHarrSemicolonEntityName, 7, nvHarrSemicolonEntityValue, 1 },
    { nvapSemicolonEntityName, 5, nvapSemicolonEntityValue, 2 },
    { nvdashSemicolonEntityName, 7, nvdashSemicolonEntityValue, 1 },
    { nvgeSemicolonEntityName, 5, nvgeSemicolonEntityValue, 2 },
    { nvgtSemicolonEntityName, 5, nvgtSemicolonEntityValue, 2 },
    { nvinfinSemicolonEntityName, 8, nvinfinSemicolonEntityValue, 1 },
    { nvlArrSemicolonEntityName, 7, nvlArrSemicolonEntityValue, 1 },
    { nvleSemicolonEntityName, 5, nvleSemicolonEntityValue, 2 },
    { nvltSemicolonEntityName, 5, nvltSemicolonEntityValue, 2 },
    { nvltrieSemicolonEntityName, 8, nvltrieSemicolonEntityValue, 2 },
    { nvrArrSemicolonEntityName, 7, nvrArrSemicolonEntityValue, 1 },
    { nvrtrieSemicolonEntityName, 8, nvrtrieSemicolonEntityValue, 2 },
    { nvsimSemicolonEntityName, 6, nvsimSemicolonEntityValue, 2 },
    { nwArrSemicolonEntityName, 6, nwArrSemicolonEntityValue, 1 },
    { nwarhkSemicolonEntityName, 7, nwarhkSemicolonEntityValue, 1 },
    { nwarrSemicolonEntityName, 6, nwarrSemicolonEntityValue, 1 },
    { nwarrowSemicolonEntityName, 8, nwarrowSemicolonEntityValue, 1 },
    { nwnearSemicolonEntityName, 7, nwnearSemicolonEntityValue, 1 },
    { oSSemicolonEntityName, 3, oSSemicolonEntityValue, 1 },
    { oacuteEntityName, 6, oacuteEntityValue, 1 },
    { oacuteSemicolonEntityName, 7, oacuteSemicolonEntityValue, 1 },
    { oastSemicolonEntityName, 5, oastSemicolonEntityValue, 1 },
    { ocirSemicolonEntityName, 5, ocirSemicolonEntityValue, 1 },
    { ocircEntityName, 5, ocircEntityValue, 1 },
    { ocircSemicolonEntityName, 6, ocircSemicolonEntityValue, 1 },
    { ocySemicolonEntityName, 4, ocySemicolonEntityValue, 1 },
    { odashSemicolonEntityName, 6, odashSemicolonEntityValue, 1 },
    { odblacSemicolonEntityName, 7, odblacSemicolonEntityValue, 1 },
    { odivSemicolonEntityName, 5, odivSemicolonEntityValue, 1 },
    { odotSemicolonEntityName, 5, odotSemicolonEntityValue, 1 },
    { odsoldSemicolonEntityName, 7, odsoldSemicolonEntityValue, 1 },
    { oeligSemicolonEntityName, 6, oeligSemicolonEntityValue, 1 },
    { ofcirSemicolonEntityName, 6, ofcirSemicolonEntityValue, 1 },
    { ofrSemicolonEntityName, 4, ofrSemicolonEntityValue, 1 },
    { ogonSemicolonEntityName, 5, ogonSemicolonEntityValue, 1 },
    { ograveEntityName, 6, ograveEntityValue, 1 },
    { ograveSemicolonEntityName, 7, ograveSemicolonEntityValue, 1 },
    { ogtSemicolonEntityName, 4, ogtSemicolonEntityValue, 1 },
    { ohbarSemicolonEntityName, 6, ohbarSemicolonEntityValue, 1 },
    { ohmSemicolonEntityName, 4, ohmSemicolonEntityValue, 1 },
    { ointSemicolonEntityName, 5, ointSemicolonEntityValue, 1 },
    { olarrSemicolonEntityName, 6, olarrSemicolonEntityValue, 1 },
    { olcirSemicolonEntityName, 6, olcirSemicolonEntityValue, 1 },
    { olcrossSemicolonEntityName, 8, olcrossSemicolonEntityValue, 1 },
    { olineSemicolonEntityName, 6, olineSemicolonEntityValue, 1 },
    { oltSemicolonEntityName, 4, oltSemicolonEntityValue, 1 },
    { omacrSemicolonEntityName, 6, omacrSemicolonEntityValue, 1 },
    { omegaSemicolonEntityName, 6, omegaSemicolonEntityValue, 1 },
    { omicronSemicolonEntityName, 8, omicronSemicolonEntityValue, 1 },
    { omidSemicolonEntityName, 5, omidSemicolonEntityValue, 1 },
    { ominusSemicolonEntityName, 7, ominusSemicolonEntityValue, 1 },
    { oopfSemicolonEntityName, 5, oopfSemicolonEntityValue, 1 },
    { oparSemicolonEntityName, 5, oparSemicolonEntityValue, 1 },
    { operpSemicolonEntityName, 6, operpSemicolonEntityValue, 1 },
    { oplusSemicolonEntityName, 6, oplusSemicolonEntityValue, 1 },
    { orSemicolonEntityName, 3, orSemicolonEntityValue, 1 },
    { orarrSemicolonEntityName, 6, orarrSemicolonEntityValue, 1 },
    { ordSemicolonEntityName, 4, ordSemicolonEntityValue, 1 },
    { orderSemicolonEntityName, 6, orderSemicolonEntityValue, 1 },
    { orderofSemicolonEntityName, 8, orderofSemicolonEntityValue, 1 },
    { ordfEntityName, 4, ordfEntityValue, 1 },
    { ordfSemicolonEntityName, 5, ordfSemicolonEntityValue, 1 },
    { ordmEntityName, 4, ordmEntityValue, 1 },
    { ordmSemicolonEntityName, 5, ordmSemicolonEntityValue, 1 },
    { origofSemicolonEntityName, 7, origofSemicolonEntityValue, 1 },
    { ororSemicolonEntityName, 5, ororSemicolonEntityValue, 1 },
    { orslopeSemicolonEntityName, 8, orslopeSemicolonEntityValue, 1 },
    { orvSemicolonEntityName, 4, orvSemicolonEntityValue, 1 },
    { oscrSemicolonEntityName, 5, oscrSemicolonEntityValue, 1 },
    { oslashEntityName, 6, oslashEntityValue, 1 },
    { oslashSemicolonEntityName, 7, oslashSemicolonEntityValue, 1 },
    { osolSemicolonEntityName, 5, osolSemicolonEntityValue, 1 },
    { otildeEntityName, 6, otildeEntityValue, 1 },
    { otildeSemicolonEntityName, 7, otildeSemicolonEntityValue, 1 },
    { otimesSemicolonEntityName, 7, otimesSemicolonEntityValue, 1 },
    { otimesasSemicolonEntityName, 9, otimesasSemicolonEntityValue, 1 },
    { oumlEntityName, 4, oumlEntityValue, 1 },
    { oumlSemicolonEntityName, 5, oumlSemicolonEntityValue, 1 },
    { ovbarSemicolonEntityName, 6, ovbarSemicolonEntityValue, 1 },
    { parSemicolonEntityName, 4, parSemicolonEntityValue, 1 },
    { paraEntityName, 4, paraEntityValue, 1 },
    { paraSemicolonEntityName, 5, paraSemicolonEntityValue, 1 },
    { parallelSemicolonEntityName, 9, parallelSemicolonEntityValue, 1 },
    { parsimSemicolonEntityName, 7, parsimSemicolonEntityValue, 1 },
    { parslSemicolonEntityName, 6, parslSemicolonEntityValue, 1 },
    { partSemicolonEntityName, 5, partSemicolonEntityValue, 1 },
    { pcySemicolonEntityName, 4, pcySemicolonEntityValue, 1 },
    { percntSemicolonEntityName, 7, percntSemicolonEntityValue, 1 },
    { periodSemicolonEntityName, 7, periodSemicolonEntityValue, 1 },
    { permilSemicolonEntityName, 7, permilSemicolonEntityValue, 1 },
    { perpSemicolonEntityName, 5, perpSemicolonEntityValue, 1 },
    { pertenkSemicolonEntityName, 8, pertenkSemicolonEntityValue, 1 },
    { pfrSemicolonEntityName, 4, pfrSemicolonEntityValue, 1 },
    { phiSemicolonEntityName, 4, phiSemicolonEntityValue, 1 },
    { phivSemicolonEntityName, 5, phivSemicolonEntityValue, 1 },
    { phmmatSemicolonEntityName, 7, phmmatSemicolonEntityValue, 1 },
    { phoneSemicolonEntityName, 6, phoneSemicolonEntityValue, 1 },
    { piSemicolonEntityName, 3, piSemicolonEntityValue, 1 },
    { pitchforkSemicolonEntityName, 10, pitchforkSemicolonEntityValue, 1 },
    { pivSemicolonEntityName, 4, pivSemicolonEntityValue, 1 },
    { planckSemicolonEntityName, 7, planckSemicolonEntityValue, 1 },
    { planckhSemicolonEntityName, 8, planckhSemicolonEntityValue, 1 },
    { plankvSemicolonEntityName, 7, plankvSemicolonEntityValue, 1 },
    { plusSemicolonEntityName, 5, plusSemicolonEntityValue, 1 },
    { plusacirSemicolonEntityName, 9, plusacirSemicolonEntityValue, 1 },
    { plusbSemicolonEntityName, 6, plusbSemicolonEntityValue, 1 },
    { pluscirSemicolonEntityName, 8, pluscirSemicolonEntityValue, 1 },
    { plusdoSemicolonEntityName, 7, plusdoSemicolonEntityValue, 1 },
    { plusduSemicolonEntityName, 7, plusduSemicolonEntityValue, 1 },
    { pluseSemicolonEntityName, 6, pluseSemicolonEntityValue, 1 },
    { plusmnEntityName, 6, plusmnEntityValue, 1 },
    { plusmnSemicolonEntityName, 7, plusmnSemicolonEntityValue, 1 },
    { plussimSemicolonEntityName, 8, plussimSemicolonEntityValue, 1 },
    { plustwoSemicolonEntityName, 8, plustwoSemicolonEntityValue, 1 },
    { pmSemicolonEntityName, 3, pmSemicolonEntityValue, 1 },
    { pointintSemicolonEntityName, 9, pointintSemicolonEntityValue, 1 },
    { popfSemicolonEntityName, 5, popfSemicolonEntityValue, 1 },
    { poundEntityName, 5, poundEntityValue, 1 },
    { poundSemicolonEntityName, 6, poundSemicolonEntityValue, 1 },
    { prSemicolonEntityName, 3, prSemicolonEntityValue, 1 },
    { prESemicolonEntityName, 4, prESemicolonEntityValue, 1 },
    { prapSemicolonEntityName, 5, prapSemicolonEntityValue, 1 },
    { prcueSemicolonEntityName, 6, prcueSemicolonEntityValue, 1 },
    { preSemicolonEntityName, 4, preSemicolonEntityValue, 1 },
    { precSemicolonEntityName, 5, precSemicolonEntityValue, 1 },
    { precapproxSemicolonEntityName, 11, precapproxSemicolonEntityValue, 1 },
    { preccurlyeqSemicolonEntityName, 12, preccurlyeqSemicolonEntityValue, 1 },
    { preceqSemicolonEntityName, 7, preceqSemicolonEntityValue, 1 },
    { precnapproxSemicolonEntityName, 12, precnapproxSemicolonEntityValue, 1 },
    { precneqqSemicolonEntityName, 9, precneqqSemicolonEntityValue, 1 },
    { precnsimSemicolonEntityName, 9, precnsimSemicolonEntityValue, 1 },
    { precsimSemicolonEntityName, 8, precsimSemicolonEntityValue, 1 },
    { primeSemicolonEntityName, 6, primeSemicolonEntityValue, 1 },
    { primesSemicolonEntityName, 7, primesSemicolonEntityValue, 1 },
    { prnESemicolonEntityName, 5, prnESemicolonEntityValue, 1 },
    { prnapSemicolonEntityName, 6, prnapSemicolonEntityValue, 1 },
    { prnsimSemicolonEntityName, 7, prnsimSemicolonEntityValue, 1 },
    { prodSemicolonEntityName, 5, prodSemicolonEntityValue, 1 },
    { profalarSemicolonEntityName, 9, profalarSemicolonEntityValue, 1 },
    { proflineSemicolonEntityName, 9, proflineSemicolonEntityValue, 1 },
    { profsurfSemicolonEntityName, 9, profsurfSemicolonEntityValue, 1 },
    { propSemicolonEntityName, 5, propSemicolonEntityValue, 1 },
    { proptoSemicolonEntityName, 7, proptoSemicolonEntityValue, 1 },
    { prsimSemicolonEntityName, 6, prsimSemicolonEntityValue, 1 },
    { prurelSemicolonEntityName, 7, prurelSemicolonEntityValue, 1 },
    { pscrSemicolonEntityName, 5, pscrSemicolonEntityValue, 1 },
    { psiSemicolonEntityName, 4, psiSemicolonEntityValue, 1 },
    { puncspSemicolonEntityName, 7, puncspSemicolonEntityValue, 1 },
    { qfrSemicolonEntityName, 4, qfrSemicolonEntityValue, 1 },
    { qintSemicolonEntityName, 5, qintSemicolonEntityValue, 1 },
    { qopfSemicolonEntityName, 5, qopfSemicolonEntityValue, 1 },
    { qprimeSemicolonEntityName, 7, qprimeSemicolonEntityValue, 1 },
    { qscrSemicolonEntityName, 5, qscrSemicolonEntityValue, 1 },
    { quaternionsSemicolonEntityName, 12, quaternionsSemicolonEntityValue, 1 },
    { quatintSemicolonEntityName, 8, quatintSemicolonEntityValue, 1 },
    { questSemicolonEntityName, 6, questSemicolonEntityValue, 1 },
    { questeqSemicolonEntityName, 8, questeqSemicolonEntityValue, 1 },
    { quotEntityName, 4, quotEntityValue, 1 },
    { quotSemicolonEntityName, 5, quotSemicolonEntityValue, 1 },
    { rAarrSemicolonEntityName, 6, rAarrSemicolonEntityValue, 1 },
    { rArrSemicolonEntityName, 5, rArrSemicolonEntityValue, 1 },
    { rAtailSemicolonEntityName, 7, rAtailSemicolonEntityValue, 1 },
    { rBarrSemicolonEntityName, 6, rBarrSemicolonEntityValue, 1 },
    { rHarSemicolonEntityName, 5, rHarSemicolonEntityValue, 1 },
    { raceSemicolonEntityName, 5, raceSemicolonEntityValue, 2 },
    { racuteSemicolonEntityName, 7, racuteSemicolonEntityValue, 1 },
    { radicSemicolonEntityName, 6, radicSemicolonEntityValue, 1 },
    { raemptyvSemicolonEntityName, 9, raemptyvSemicolonEntityValue, 1 },
    { rangSemicolonEntityName, 5, rangSemicolonEntityValue, 1 },
    { rangdSemicolonEntityName, 6, rangdSemicolonEntityValue, 1 },
    { rangeSemicolonEntityName, 6, rangeSemicolonEntityValue, 1 },
    { rangleSemicolonEntityName, 7, rangleSemicolonEntityValue, 1 },
    { raquoEntityName, 5, raquoEntityValue, 1 },
    { raquoSemicolonEntityName, 6, raquoSemicolonEntityValue, 1 },
    { rarrSemicolonEntityName, 5, rarrSemicolonEntityValue, 1 },
    { rarrapSemicolonEntityName, 7, rarrapSemicolonEntityValue, 1 },
    { rarrbSemicolonEntityName, 6, rarrbSemicolonEntityValue, 1 },
    { rarrbfsSemicolonEntityName, 8, rarrbfsSemicolonEntityValue, 1 },
    { rarrcSemicolonEntityName, 6, rarrcSemicolonEntityValue, 1 },
    { rarrfsSemicolonEntityName, 7, rarrfsSemicolonEntityValue, 1 },
    { rarrhkSemicolonEntityName, 7, rarrhkSemicolonEntityValue, 1 },
    { rarrlpSemicolonEntityName, 7, rarrlpSemicolonEntityValue, 1 },
    { rarrplSemicolonEntityName, 7, rarrplSemicolonEntityValue, 1 },
    { rarrsimSemicolonEntityName, 8, rarrsimSemicolonEntityValue, 1 },
    { rarrtlSemicolonEntityName, 7, rarrtlSemicolonEntityValue, 1 },
    { rarrwSemicolonEntityName, 6, rarrwSemicolonEntityValue, 1 },
    { ratailSemicolonEntityName, 7, ratailSemicolonEntityValue, 1 },
    { ratioSemicolonEntityName, 6, ratioSemicolonEntityValue, 1 },
    { rationalsSemicolonEntityName, 10, rationalsSemicolonEntityValue, 1 },
    { rbarrSemicolonEntityName, 6, rbarrSemicolonEntityValue, 1 },
    { rbbrkSemicolonEntityName, 6, rbbrkSemicolonEntityValue, 1 },
    { rbraceSemicolonEntityName, 7, rbraceSemicolonEntityValue, 1 },
    { rbrackSemicolonEntityName, 7, rbrackSemicolonEntityValue, 1 },
    { rbrkeSemicolonEntityName, 6, rbrkeSemicolonEntityValue, 1 },
    { rbrksldSemicolonEntityName, 8, rbrksldSemicolonEntityValue, 1 },
    { rbrksluSemicolonEntityName, 8, rbrksluSemicolonEntityValue, 1 },
    { rcaronSemicolonEntityName, 7, rcaronSemicolonEntityValue, 1 },
    { rcedilSemicolonEntityName, 7, rcedilSemicolonEntityValue, 1 },
    { rceilSemicolonEntityName, 6, rceilSemicolonEntityValue, 1 },
    { rcubSemicolonEntityName, 5, rcubSemicolonEntityValue, 1 },
    { rcySemicolonEntityName, 4, rcySemicolonEntityValue, 1 },
    { rdcaSemicolonEntityName, 5, rdcaSemicolonEntityValue, 1 },
    { rdldharSemicolonEntityName, 8, rdldharSemicolonEntityValue, 1 },
    { rdquoSemicolonEntityName, 6, rdquoSemicolonEntityValue, 1 },
    { rdquorSemicolonEntityName, 7, rdquorSemicolonEntityValue, 1 },
    { rdshSemicolonEntityName, 5, rdshSemicolonEntityValue, 1 },
    { realSemicolonEntityName, 5, realSemicolonEntityValue, 1 },
    { realineSemicolonEntityName, 8, realineSemicolonEntityValue, 1 },
    { realpartSemicolonEntityName, 9, realpartSemicolonEntityValue, 1 },
    { realsSemicolonEntityName, 6, realsSemicolonEntityValue, 1 },
    { rectSemicolonEntityName, 5, rectSemicolonEntityValue, 1 },
    { regEntityName, 3, regEntityValue, 1 },
    { regSemicolonEntityName, 4, regSemicolonEntityValue, 1 },
    { rfishtSemicolonEntityName, 7, rfishtSemicolonEntityValue, 1 },
    { rfloorSemicolonEntityName, 7, rfloorSemicolonEntityValue, 1 },
    { rfrSemicolonEntityName, 4, rfrSemicolonEntityValue, 1 },
    { rhardSemicolonEntityName, 6, rhardSemicolonEntityValue, 1 },
    { rharuSemicolonEntityName, 6, rharuSemicolonEntityValue, 1 },
    { rharulSemicolonEntityName, 7, rharulSemicolonEntityValue, 1 },
    { rhoSemicolonEntityName, 4, rhoSemicolonEntityValue, 1 },
    { rhovSemicolonEntityName, 5, rhovSemicolonEntityValue, 1 },
    { rightarrowSemicolonEntityName, 11, rightarrowSemicolonEntityValue, 1 },
    { rightarrowtailSemicolonEntityName, 15, rightarrowtailSemicolonEntityValue, 1 },
    { rightharpoondownSemicolonEntityName, 17, rightharpoondownSemicolonEntityValue, 1 },
    { rightharpoonupSemicolonEntityName, 15, rightharpoonupSemicolonEntityValue, 1 },
    { rightleftarrowsSemicolonEntityName, 16, rightleftarrowsSemicolonEntityValue, 1 },
    { rightleftharpoonsSemicolonEntityName, 18, rightleftharpoonsSemicolonEntityValue, 1 },
    { rightrightarrowsSemicolonEntityName, 17, rightrightarrowsSemicolonEntityValue, 1 },
    { rightsquigarrowSemicolonEntityName, 16, rightsquigarrowSemicolonEntityValue, 1 },
    { rightthreetimesSemicolonEntityName, 16, rightthreetimesSemicolonEntityValue, 1 },
    { ringSemicolonEntityName, 5, ringSemicolonEntityValue, 1 },
    { risingdotseqSemicolonEntityName, 13, risingdotseqSemicolonEntityValue, 1 },
    { rlarrSemicolonEntityName, 6, rlarrSemicolonEntityValue, 1 },
    { rlharSemicolonEntityName, 6, rlharSemicolonEntityValue, 1 },
    { rlmSemicolonEntityName, 4, rlmSemicolonEntityValue, 1 },
    { rmoustSemicolonEntityName, 7, rmoustSemicolonEntityValue, 1 },
    { rmoustacheSemicolonEntityName, 11, rmoustacheSemicolonEntityValue, 1 },
    { rnmidSemicolonEntityName, 6, rnmidSemicolonEntityValue, 1 },
    { roangSemicolonEntityName, 6, roangSemicolonEntityValue, 1 },
    { roarrSemicolonEntityName, 6, roarrSemicolonEntityValue, 1 },
    { robrkSemicolonEntityName, 6, robrkSemicolonEntityValue, 1 },
    { roparSemicolonEntityName, 6, roparSemicolonEntityValue, 1 },
    { ropfSemicolonEntityName, 5, ropfSemicolonEntityValue, 1 },
    { roplusSemicolonEntityName, 7, roplusSemicolonEntityValue, 1 },
    { rotimesSemicolonEntityName, 8, rotimesSemicolonEntityValue, 1 },
    { rparSemicolonEntityName, 5, rparSemicolonEntityValue, 1 },
    { rpargtSemicolonEntityName, 7, rpargtSemicolonEntityValue, 1 },
    { rppolintSemicolonEntityName, 9, rppolintSemicolonEntityValue, 1 },
    { rrarrSemicolonEntityName, 6, rrarrSemicolonEntityValue, 1 },
    { rsaquoSemicolonEntityName, 7, rsaquoSemicolonEntityValue, 1 },
    { rscrSemicolonEntityName, 5, rscrSemicolonEntityValue, 1 },
    { rshSemicolonEntityName, 4, rshSemicolonEntityValue, 1 },
    { rsqbSemicolonEntityName, 5, rsqbSemicolonEntityValue, 1 },
    { rsquoSemicolonEntityName, 6, rsquoSemicolonEntityValue, 1 },
    { rsquorSemicolonEntityName, 7, rsquorSemicolonEntityValue, 1 },
    { rthreeSemicolonEntityName, 7, rthreeSemicolonEntityValue, 1 },
    { rtimesSemicolonEntityName, 7, rtimesSemicolonEntityValue, 1 },
    { rtriSemicolonEntityName, 5, rtriSemicolonEntityValue, 1 },
    { rtrieSemicolonEntityName, 6, rtrieSemicolonEntityValue, 1 },
    { rtrifSemicolonEntityName, 6, rtrifSemicolonEntityValue, 1 },
    { rtriltriSemicolonEntityName, 9, rtriltriSemicolonEntityValue, 1 },
    { ruluharSemicolonEntityName, 8, ruluharSemicolonEntityValue, 1 },
    { rxSemicolonEntityName, 3, rxSemicolonEntityValue, 1 },
    { sacuteSemicolonEntityName, 7, sacuteSemicolonEntityValue, 1 },
    { sbquoSemicolonEntityName, 6, sbquoSemicolonEntityValue, 1 },
    { scSemicolonEntityName, 3, scSemicolonEntityValue, 1 },
    { scESemicolonEntityName, 4, scESemicolonEntityValue, 1 },
    { scapSemicolonEntityName, 5, scapSemicolonEntityValue, 1 },
    { scaronSemicolonEntityName, 7, scaronSemicolonEntityValue, 1 },
    { sccueSemicolonEntityName, 6, sccueSemicolonEntityValue, 1 },
    { sceSemicolonEntityName, 4, sceSemicolonEntityValue, 1 },
    { scedilSemicolonEntityName, 7, scedilSemicolonEntityValue, 1 },
    { scircSemicolonEntityName, 6, scircSemicolonEntityValue, 1 },
    { scnESemicolonEntityName, 5, scnESemicolonEntityValue, 1 },
    { scnapSemicolonEntityName, 6, scnapSemicolonEntityValue, 1 },
    { scnsimSemicolonEntityName, 7, scnsimSemicolonEntityValue, 1 },
    { scpolintSemicolonEntityName, 9, scpolintSemicolonEntityValue, 1 },
    { scsimSemicolonEntityName, 6, scsimSemicolonEntityValue, 1 },
    { scySemicolonEntityName, 4, scySemicolonEntityValue, 1 },
    { sdotSemicolonEntityName, 5, sdotSemicolonEntityValue, 1 },
    { sdotbSemicolonEntityName, 6, sdotbSemicolonEntityValue, 1 },
    { sdoteSemicolonEntityName, 6, sdoteSemicolonEntityValue, 1 },
    { seArrSemicolonEntityName, 6, seArrSemicolonEntityValue, 1 },
    { searhkSemicolonEntityName, 7, searhkSemicolonEntityValue, 1 },
    { searrSemicolonEntityName, 6, searrSemicolonEntityValue, 1 },
    { searrowSemicolonEntityName, 8, searrowSemicolonEntityValue, 1 },
    { sectEntityName, 4, sectEntityValue, 1 },
    { sectSemicolonEntityName, 5, sectSemicolonEntityValue, 1 },
    { semiSemicolonEntityName, 5, semiSemicolonEntityValue, 1 },
    { seswarSemicolonEntityName, 7, seswarSemicolonEntityValue, 1 },
    { setminusSemicolonEntityName, 9, setminusSemicolonEntityValue, 1 },
    { setmnSemicolonEntityName, 6, setmnSemicolonEntityValue, 1 },
    { sextSemicolonEntityName, 5, sextSemicolonEntityValue, 1 },
    { sfrSemicolonEntityName, 4, sfrSemicolonEntityValue, 1 },
    { sfrownSemicolonEntityName, 7, sfrownSemicolonEntityValue, 1 },
    { sharpSemicolonEntityName, 6, sharpSemicolonEntityValue, 1 },
    { shchcySemicolonEntityName, 7, shchcySemicolonEntityValue, 1 },
    { shcySemicolonEntityName, 5, shcySemicolonEntityValue, 1 },
    { shortmidSemicolonEntityName, 9, shortmidSemicolonEntityValue, 1 },
    { shortparallelSemicolonEntityName, 14, shortparallelSemicolonEntityValue, 1 },
    { shyEntityName, 3, shyEntityValue, 1 },
    { shySemicolonEntityName, 4, shySemicolonEntityValue, 1 },
    { sigmaSemicolonEntityName, 6, sigmaSemicolonEntityValue, 1 },
    { sigmafSemicolonEntityName, 7, sigmafSemicolonEntityValue, 1 },
    { sigmavSemicolonEntityName, 7, sigmavSemicolonEntityValue, 1 },
    { simSemicolonEntityName, 4, simSemicolonEntityValue, 1 },
    { simdotSemicolonEntityName, 7, simdotSemicolonEntityValue, 1 },
    { simeSemicolonEntityName, 5, simeSemicolonEntityValue, 1 },
    { simeqSemicolonEntityName, 6, simeqSemicolonEntityValue, 1 },
    { simgSemicolonEntityName, 5, simgSemicolonEntityValue, 1 },
    { simgESemicolonEntityName, 6, simgESemicolonEntityValue, 1 },
    { simlSemicolonEntityName, 5, simlSemicolonEntityValue, 1 },
    { simlESemicolonEntityName, 6, simlESemicolonEntityValue, 1 },
    { simneSemicolonEntityName, 6, simneSemicolonEntityValue, 1 },
    { simplusSemicolonEntityName, 8, simplusSemicolonEntityValue, 1 },
    { simrarrSemicolonEntityName, 8, simrarrSemicolonEntityValue, 1 },
    { slarrSemicolonEntityName, 6, slarrSemicolonEntityValue, 1 },
    { smallsetminusSemicolonEntityName, 14, smallsetminusSemicolonEntityValue, 1 },
    { smashpSemicolonEntityName, 7, smashpSemicolonEntityValue, 1 },
    { smeparslSemicolonEntityName, 9, smeparslSemicolonEntityValue, 1 },
    { smidSemicolonEntityName, 5, smidSemicolonEntityValue, 1 },
    { smileSemicolonEntityName, 6, smileSemicolonEntityValue, 1 },
    { smtSemicolonEntityName, 4, smtSemicolonEntityValue, 1 },
    { smteSemicolonEntityName, 5, smteSemicolonEntityValue, 1 },
    { smtesSemicolonEntityName, 6, smtesSemicolonEntityValue, 2 },
    { softcySemicolonEntityName, 7, softcySemicolonEntityValue, 1 },
    { solSemicolonEntityName, 4, solSemicolonEntityValue, 1 },
    { solbSemicolonEntityName, 5, solbSemicolonEntityValue, 1 },
    { solbarSemicolonEntityName, 7, solbarSemicolonEntityValue, 1 },
    { sopfSemicolonEntityName, 5, sopfSemicolonEntityValue, 1 },
    { spadesSemicolonEntityName, 7, spadesSemicolonEntityValue, 1 },
    { spadesuitSemicolonEntityName, 10, spadesuitSemicolonEntityValue, 1 },
    { sparSemicolonEntityName, 5, sparSemicolonEntityValue, 1 },
    { sqcapSemicolonEntityName, 6, sqcapSemicolonEntityValue, 1 },
    { sqcapsSemicolonEntityName, 7, sqcapsSemicolonEntityValue, 2 },
    { sqcupSemicolonEntityName, 6, sqcupSemicolonEntityValue, 1 },
    { sqcupsSemicolonEntityName, 7, sqcupsSemicolonEntityValue, 2 },
    { sqsubSemicolonEntityName, 6, sqsubSemicolonEntityValue, 1 },
    { sqsubeSemicolonEntityName, 7, sqsubeSemicolonEntityValue, 1 },
    { sqsubsetSemicolonEntityName, 9, sqsubsetSemicolonEntityValue, 1 },
    { sqsubseteqSemicolonEntityName, 11, sqsubseteqSemicolonEntityValue, 1 },
    { sqsupSemicolonEntityName, 6, sqsupSemicolonEntityValue, 1 },
    { sqsupeSemicolonEntityName, 7, sqsupeSemicolonEntityValue, 1 },
    { sqsupsetSemicolonEntityName, 9, sqsupsetSemicolonEntityValue, 1 },
    { sqsupseteqSemicolonEntityName, 11, sqsupseteqSemicolonEntityValue, 1 },
    { squSemicolonEntityName, 4, squSemicolonEntityValue, 1 },
    { squareSemicolonEntityName, 7, squareSemicolonEntityValue, 1 },
    { squarfSemicolonEntityName, 7, squarfSemicolonEntityValue, 1 },
    { squfSemicolonEntityName, 5, squfSemicolonEntityValue, 1 },
    { srarrSemicolonEntityName, 6, srarrSemicolonEntityValue, 1 },
    { sscrSemicolonEntityName, 5, sscrSemicolonEntityValue, 1 },
    { ssetmnSemicolonEntityName, 7, ssetmnSemicolonEntityValue, 1 },
    { ssmileSemicolonEntityName, 7, ssmileSemicolonEntityValue, 1 },
    { sstarfSemicolonEntityName, 7, sstarfSemicolonEntityValue, 1 },
    { starSemicolonEntityName, 5, starSemicolonEntityValue, 1 },
    { starfSemicolonEntityName, 6, starfSemicolonEntityValue, 1 },
    { straightepsilonSemicolonEntityName, 16, straightepsilonSemicolonEntityValue, 1 },
    { straightphiSemicolonEntityName, 12, straightphiSemicolonEntityValue, 1 },
    { strnsSemicolonEntityName, 6, strnsSemicolonEntityValue, 1 },
    { subSemicolonEntityName, 4, subSemicolonEntityValue, 1 },
    { subESemicolonEntityName, 5, subESemicolonEntityValue, 1 },
    { subdotSemicolonEntityName, 7, subdotSemicolonEntityValue, 1 },
    { subeSemicolonEntityName, 5, subeSemicolonEntityValue, 1 },
    { subedotSemicolonEntityName, 8, subedotSemicolonEntityValue, 1 },
    { submultSemicolonEntityName, 8, submultSemicolonEntityValue, 1 },
    { subnESemicolonEntityName, 6, subnESemicolonEntityValue, 1 },
    { subneSemicolonEntityName, 6, subneSemicolonEntityValue, 1 },
    { subplusSemicolonEntityName, 8, subplusSemicolonEntityValue, 1 },
    { subrarrSemicolonEntityName, 8, subrarrSemicolonEntityValue, 1 },
    { subsetSemicolonEntityName, 7, subsetSemicolonEntityValue, 1 },
    { subseteqSemicolonEntityName, 9, subseteqSemicolonEntityValue, 1 },
    { subseteqqSemicolonEntityName, 10, subseteqqSemicolonEntityValue, 1 },
    { subsetneqSemicolonEntityName, 10, subsetneqSemicolonEntityValue, 1 },
    { subsetneqqSemicolonEntityName, 11, subsetneqqSemicolonEntityValue, 1 },
    { subsimSemicolonEntityName, 7, subsimSemicolonEntityValue, 1 },
    { subsubSemicolonEntityName, 7, subsubSemicolonEntityValue, 1 },
    { subsupSemicolonEntityName, 7, subsupSemicolonEntityValue, 1 },
    { succSemicolonEntityName, 5, succSemicolonEntityValue, 1 },
    { succapproxSemicolonEntityName, 11, succapproxSemicolonEntityValue, 1 },
    { succcurlyeqSemicolonEntityName, 12, succcurlyeqSemicolonEntityValue, 1 },
    { succeqSemicolonEntityName, 7, succeqSemicolonEntityValue, 1 },
    { succnapproxSemicolonEntityName, 12, succnapproxSemicolonEntityValue, 1 },
    { succneqqSemicolonEntityName, 9, succneqqSemicolonEntityValue, 1 },
    { succnsimSemicolonEntityName, 9, succnsimSemicolonEntityValue, 1 },
    { succsimSemicolonEntityName, 8, succsimSemicolonEntityValue, 1 },
    { sumSemicolonEntityName, 4, sumSemicolonEntityValue, 1 },
    { sungSemicolonEntityName, 5, sungSemicolonEntityValue, 1 },
    { sup1EntityName, 4, sup1EntityValue, 1 },
    { sup1SemicolonEntityName, 5, sup1SemicolonEntityValue, 1 },
    { sup2EntityName, 4, sup2EntityValue, 1 },
    { sup2SemicolonEntityName, 5, sup2SemicolonEntityValue, 1 },
    { sup3EntityName, 4, sup3EntityValue, 1 },
    { sup3SemicolonEntityName, 5, sup3SemicolonEntityValue, 1 },
    { supSemicolonEntityName, 4, supSemicolonEntityValue, 1 },
    { supESemicolonEntityName, 5, supESemicolonEntityValue, 1 },
    { supdotSemicolonEntityName, 7, supdotSemicolonEntityValue, 1 },
    { supdsubSemicolonEntityName, 8, supdsubSemicolonEntityValue, 1 },
    { supeSemicolonEntityName, 5, supeSemicolonEntityValue, 1 },
    { supedotSemicolonEntityName, 8, supedotSemicolonEntityValue, 1 },
    { suphsolSemicolonEntityName, 8, suphsolSemicolonEntityValue, 1 },
    { suphsubSemicolonEntityName, 8, suphsubSemicolonEntityValue, 1 },
    { suplarrSemicolonEntityName, 8, suplarrSemicolonEntityValue, 1 },
    { supmultSemicolonEntityName, 8, supmultSemicolonEntityValue, 1 },
    { supnESemicolonEntityName, 6, supnESemicolonEntityValue, 1 },
    { supneSemicolonEntityName, 6, supneSemicolonEntityValue, 1 },
    { supplusSemicolonEntityName, 8, supplusSemicolonEntityValue, 1 },
    { supsetSemicolonEntityName, 7, supsetSemicolonEntityValue, 1 },
    { supseteqSemicolonEntityName, 9, supseteqSemicolonEntityValue, 1 },
    { supseteqqSemicolonEntityName, 10, supseteqqSemicolonEntityValue, 1 },
    { supsetneqSemicolonEntityName, 10, supsetneqSemicolonEntityValue, 1 },
    { supsetneqqSemicolonEntityName, 11, supsetneqqSemicolonEntityValue, 1 },
    { supsimSemicolonEntityName, 7, supsimSemicolonEntityValue, 1 },
    { supsubSemicolonEntityName, 7, supsubSemicolonEntityValue, 1 },
    { supsupSemicolonEntityName, 7, supsupSemicolonEntityValue, 1 },
    { swArrSemicolonEntityName, 6, swArrSemicolonEntityValue, 1 },
    { swarhkSemicolonEntityName, 7, swarhkSemicolonEntityValue, 1 },
    { swarrSemicolonEntityName, 6, swarrSemicolonEntityValue, 1 },
    { swarrowSemicolonEntityName, 8, swarrowSemicolonEntityValue, 1 },
    { swnwarSemicolonEntityName, 7, swnwarSemicolonEntityValue, 1 },
    { szligEntityName, 5, szligEntityValue, 1 },
    { szligSemicolonEntityName, 6, szligSemicolonEntityValue, 1 },
    { targetSemicolonEntityName, 7, targetSemicolonEntityValue, 1 },
    { tauSemicolonEntityName, 4, tauSemicolonEntityValue, 1 },
    { tbrkSemicolonEntityName, 5, tbrkSemicolonEntityValue, 1 },
    { tcaronSemicolonEntityName, 7, tcaronSemicolonEntityValue, 1 },
    { tcedilSemicolonEntityName, 7, tcedilSemicolonEntityValue, 1 },
    { tcySemicolonEntityName, 4, tcySemicolonEntityValue, 1 },
    { tdotSemicolonEntityName, 5, tdotSemicolonEntityValue, 1 },
    { telrecSemicolonEntityName, 7, telrecSemicolonEntityValue, 1 },
    { tfrSemicolonEntityName, 4, tfrSemicolonEntityValue, 1 },
    { there4SemicolonEntityName, 7, there4SemicolonEntityValue, 1 },
    { thereforeSemicolonEntityName, 10, thereforeSemicolonEntityValue, 1 },
    { thetaSemicolonEntityName, 6, thetaSemicolonEntityValue, 1 },
    { thetasymSemicolonEntityName, 9, thetasymSemicolonEntityValue, 1 },
    { thetavSemicolonEntityName, 7, thetavSemicolonEntityValue, 1 },
    { thickapproxSemicolonEntityName, 12, thickapproxSemicolonEntityValue, 1 },
    { thicksimSemicolonEntityName, 9, thicksimSemicolonEntityValue, 1 },
    { thinspSemicolonEntityName, 7, thinspSemicolonEntityValue, 1 },
    { thkapSemicolonEntityName, 6, thkapSemicolonEntityValue, 1 },
    { thksimSemicolonEntityName, 7, thksimSemicolonEntityValue, 1 },
    { thornEntityName, 5, thornEntityValue, 1 },
    { thornSemicolonEntityName, 6, thornSemicolonEntityValue, 1 },
    { tildeSemicolonEntityName, 6, tildeSemicolonEntityValue, 1 },
    { timesEntityName, 5, timesEntityValue, 1 },
    { timesSemicolonEntityName, 6, timesSemicolonEntityValue, 1 },
    { timesbSemicolonEntityName, 7, timesbSemicolonEntityValue, 1 },
    { timesbarSemicolonEntityName, 9, timesbarSemicolonEntityValue, 1 },
    { timesdSemicolonEntityName, 7, timesdSemicolonEntityValue, 1 },
    { tintSemicolonEntityName, 5, tintSemicolonEntityValue, 1 },
    { toeaSemicolonEntityName, 5, toeaSemicolonEntityValue, 1 },
    { topSemicolonEntityName, 4, topSemicolonEntityValue, 1 },
    { topbotSemicolonEntityName, 7, topbotSemicolonEntityValue, 1 },
    { topcirSemicolonEntityName, 7, topcirSemicolonEntityValue, 1 },
    { topfSemicolonEntityName, 5, topfSemicolonEntityValue, 1 },
    { topforkSemicolonEntityName, 8, topforkSemicolonEntityValue, 1 },
    { tosaSemicolonEntityName, 5, tosaSemicolonEntityValue, 1 },
    { tprimeSemicolonEntityName, 7, tprimeSemicolonEntityValue, 1 },
    { tradeSemicolonEntityName, 6, tradeSemicolonEntityValue, 1 },
    { triangleSemicolonEntityName, 9, triangleSemicolonEntityValue, 1 },
    { triangledownSemicolonEntityName, 13, triangledownSemicolonEntityValue, 1 },
    { triangleleftSemicolonEntityName, 13, triangleleftSemicolonEntityValue, 1 },
    { trianglelefteqSemicolonEntityName, 15, trianglelefteqSemicolonEntityValue, 1 },
    { triangleqSemicolonEntityName, 10, triangleqSemicolonEntityValue, 1 },
    { trianglerightSemicolonEntityName, 14, trianglerightSemicolonEntityValue, 1 },
    { trianglerighteqSemicolonEntityName, 16, trianglerighteqSemicolonEntityValue, 1 },
    { tridotSemicolonEntityName, 7, tridotSemicolonEntityValue, 1 },
    { trieSemicolonEntityName, 5, trieSemicolonEntityValue, 1 },
    { triminusSemicolonEntityName, 9, triminusSemicolonEntityValue, 1 },
    { triplusSemicolonEntityName, 8, triplusSemicolonEntityValue, 1 },
    { trisbSemicolonEntityName, 6, trisbSemicolonEntityValue, 1 },
    { tritimeSemicolonEntityName, 8, tritimeSemicolonEntityValue, 1 },
    { trpeziumSemicolonEntityName, 9, trpeziumSemicolonEntityValue, 1 },
    { tscrSemicolonEntityName, 5, tscrSemicolonEntityValue, 1 },
    { tscySemicolonEntityName, 5, tscySemicolonEntityValue, 1 },
    { tshcySemicolonEntityName, 6, tshcySemicolonEntityValue, 1 },
    { tstrokSemicolonEntityName, 7, tstrokSemicolonEntityValue, 1 },
    { twixtSemicolonEntityName, 6, twixtSemicolonEntityValue, 1 },
    { twoheadleftarrowSemicolonEntityName, 17, twoheadleftarrowSemicolonEntityValue, 1 },
    { twoheadrightarrowSemicolonEntityName, 18, twoheadrightarrowSemicolonEntityValue, 1 },
    { uArrSemicolonEntityName, 5, uArrSemicolonEntityValue, 1 },
    { uHarSemicolonEntityName, 5, uHarSemicolonEntityValue, 1 },
    { uacuteEntityName, 6, uacuteEntityValue, 1 },
    { uacuteSemicolonEntityName, 7, uacuteSemicolonEntityValue, 1 },
    { uarrSemicolonEntityName, 5, uarrSemicolonEntityValue, 1 },
    { ubrcySemicolonEntityName, 6, ubrcySemicolonEntityValue, 1 },
    { ubreveSemicolonEntityName, 7, ubreveSemicolonEntityValue, 1 },
    { ucircEntityName, 5, ucircEntityValue, 1 },
    { ucircSemicolonEntityName, 6, ucircSemicolonEntityValue, 1 },
    { ucySemicolonEntityName, 4, ucySemicolonEntityValue, 1 },
    { udarrSemicolonEntityName, 6, udarrSemicolonEntityValue, 1 },
    { udblacSemicolonEntityName, 7, udblacSemicolonEntityValue, 1 },
    { udharSemicolonEntityName, 6, udharSemicolonEntityValue, 1 },
    { ufishtSemicolonEntityName, 7, ufishtSemicolonEntityValue, 1 },
    { ufrSemicolonEntityName, 4, ufrSemicolonEntityValue, 1 },
    { ugraveEntityName, 6, ugraveEntityValue, 1 },
    { ugraveSemicolonEntityName, 7, ugraveSemicolonEntityValue, 1 },
    { uharlSemicolonEntityName, 6, uharlSemicolonEntityValue, 1 },
    { uharrSemicolonEntityName, 6, uharrSemicolonEntityValue, 1 },
    { uhblkSemicolonEntityName, 6, uhblkSemicolonEntityValue, 1 },
    { ulcornSemicolonEntityName, 7, ulcornSemicolonEntityValue, 1 },
    { ulcornerSemicolonEntityName, 9, ulcornerSemicolonEntityValue, 1 },
    { ulcropSemicolonEntityName, 7, ulcropSemicolonEntityValue, 1 },
    { ultriSemicolonEntityName, 6, ultriSemicolonEntityValue, 1 },
    { umacrSemicolonEntityName, 6, umacrSemicolonEntityValue, 1 },
    { umlEntityName, 3, umlEntityValue, 1 },
    { umlSemicolonEntityName, 4, umlSemicolonEntityValue, 1 },
    { uogonSemicolonEntityName, 6, uogonSemicolonEntityValue, 1 },
    { uopfSemicolonEntityName, 5, uopfSemicolonEntityValue, 1 },
    { uparrowSemicolonEntityName, 8, uparrowSemicolonEntityValue, 1 },
    { updownarrowSemicolonEntityName, 12, updownarrowSemicolonEntityValue, 1 },
    { upharpoonleftSemicolonEntityName, 14, upharpoonleftSemicolonEntityValue, 1 },
    { upharpoonrightSemicolonEntityName, 15, upharpoonrightSemicolonEntityValue, 1 },
    { uplusSemicolonEntityName, 6, uplusSemicolonEntityValue, 1 },
    { upsiSemicolonEntityName, 5, upsiSemicolonEntityValue, 1 },
    { upsihSemicolonEntityName, 6, upsihSemicolonEntityValue, 1 },
    { upsilonSemicolonEntityName, 8, upsilonSemicolonEntityValue, 1 },
    { upuparrowsSemicolonEntityName, 11, upuparrowsSemicolonEntityValue, 1 },
    { urcornSemicolonEntityName, 7, urcornSemicolonEntityValue, 1 },
    { urcornerSemicolonEntityName, 9, urcornerSemicolonEntityValue, 1 },
    { urcropSemicolonEntityName, 7, urcropSemicolonEntityValue, 1 },
    { uringSemicolonEntityName, 6, uringSemicolonEntityValue, 1 },
    { urtriSemicolonEntityName, 6, urtriSemicolonEntityValue, 1 },
    { uscrSemicolonEntityName, 5, uscrSemicolonEntityValue, 1 },
    { utdotSemicolonEntityName, 6, utdotSemicolonEntityValue, 1 },
    { utildeSemicolonEntityName, 7, utildeSemicolonEntityValue, 1 },
    { utriSemicolonEntityName, 5, utriSemicolonEntityValue, 1 },
    { utrifSemicolonEntityName, 6, utrifSemicolonEntityValue, 1 },
    { uuarrSemicolonEntityName, 6, uuarrSemicolonEntityValue, 1 },
    { uumlEntityName, 4, uumlEntityValue, 1 },
    { uumlSemicolonEntityName, 5, uumlSemicolonEntityValue, 1 },
    { uwangleSemicolonEntityName, 8, uwangleSemicolonEntityValue, 1 },
    { vArrSemicolonEntityName, 5, vArrSemicolonEntityValue, 1 },
    { vBarSemicolonEntityName, 5, vBarSemicolonEntityValue, 1 },
    { vBarvSemicolonEntityName, 6, vBarvSemicolonEntityValue, 1 },
    { vDashSemicolonEntityName, 6, vDashSemicolonEntityValue, 1 },
    { vangrtSemicolonEntityName, 7, vangrtSemicolonEntityValue, 1 },
    { varepsilonSemicolonEntityName, 11, varepsilonSemicolonEntityValue, 1 },
    { varkappaSemicolonEntityName, 9, varkappaSemicolonEntityValue, 1 },
    { varnothingSemicolonEntityName, 11, varnothingSemicolonEntityValue, 1 },
    { varphiSemicolonEntityName, 7, varphiSemicolonEntityValue, 1 },
    { varpiSemicolonEntityName, 6, varpiSemicolonEntityValue, 1 },
    { varproptoSemicolonEntityName, 10, varproptoSemicolonEntityValue, 1 },
    { varrSemicolonEntityName, 5, varrSemicolonEntityValue, 1 },
    { varrhoSemicolonEntityName, 7, varrhoSemicolonEntityValue, 1 },
    { varsigmaSemicolonEntityName, 9, varsigmaSemicolonEntityValue, 1 },
    { varsubsetneqSemicolonEntityName, 13, varsubsetneqSemicolonEntityValue, 2 },
    { varsubsetneqqSemicolonEntityName, 14, varsubsetneqqSemicolonEntityValue, 2 },
    { varsupsetneqSemicolonEntityName, 13, varsupsetneqSemicolonEntityValue, 2 },
    { varsupsetneqqSemicolonEntityName, 14, varsupsetneqqSemicolonEntityValue, 2 },
    { varthetaSemicolonEntityName, 9, varthetaSemicolonEntityValue, 1 },
    { vartriangleleftSemicolonEntityName, 16, vartriangleleftSemicolonEntityValue, 1 },
    { vartrianglerightSemicolonEntityName, 17, vartrianglerightSemicolonEntityValue, 1 },
    { vcySemicolonEntityName, 4, vcySemicolonEntityValue, 1 },
    { vdashSemicolonEntityName, 6, vdashSemicolonEntityValue, 1 },
    { veeSemicolonEntityName, 4, veeSemicolonEntityValue, 1 },
    { veebarSemicolonEntityName, 7, veebarSemicolonEntityValue, 1 },
    { veeeqSemicolonEntityName, 6, veeeqSemicolonEntityValue, 1 },
    { vellipSemicolonEntityName, 7, vellipSemicolonEntityValue, 1 },
    { verbarSemicolonEntityName, 7, verbarSemicolonEntityValue, 1 },
    { vertSemicolonEntityName, 5, vertSemicolonEntityValue, 1 },
    { vfrSemicolonEntityName, 4, vfrSemicolonEntityValue, 1 },
    { vltriSemicolonEntityName, 6, vltriSemicolonEntityValue, 1 },
    { vnsubSemicolonEntityName, 6, vnsubSemicolonEntityValue, 2 },
    { vnsupSemicolonEntityName, 6, vnsupSemicolonEntityValue, 2 },
    { vopfSemicolonEntityName, 5, vopfSemicolonEntityValue, 1 },
    { vpropSemicolonEntityName, 6, vpropSemicolonEntityValue, 1 },
    { vrtriSemicolonEntityName, 6, vrtriSemicolonEntityValue, 1 },
    { vscrSemicolonEntityName, 5, vscrSemicolonEntityValue, 1 },
    { vsubnESemicolonEntityName, 7, vsubnESemicolonEntityValue, 2 },
    { vsubneSemicolonEntityName, 7, vsubneSemicolonEntityValue, 2 },
    { vsupnESemicolonEntityName, 7, vsupnESemicolonEntityValue, 2 },
    { vsupneSemicolonEntityName, 7, vsupneSemicolonEntityValue, 2 },
    { vzigzagSemicolonEntityName, 8, vzigzagSemicolonEntityValue, 1 },
    { wcircSemicolonEntityName, 6, wcircSemicolonEntityValue, 1 },
    { wedbarSemicolonEntityName, 7, wedbarSemicolonEntityValue, 1 },
    { wedgeSemicolonEntityName, 6, wedgeSemicolonEntityValue, 1 },
    { wedgeqSemicolonEntityName, 7, wedgeqSemicolonEntityValue, 1 },
    { weierpSemicolonEntityName, 7, weierpSemicolonEntityValue, 1 },
    { wfrSemicolonEntityName, 4, wfrSemicolonEntityValue, 1 },
    { wopfSemicolonEntityName, 5, wopfSemicolonEntityValue, 1 },
    { wpSemicolonEntityName, 3, wpSemicolonEntityValue, 1 },
    { wrSemicolonEntityName, 3, wrSemicolonEntityValue, 1 },
    { wreathSemicolonEntityName, 7, wreathSemicolonEntityValue, 1 },
    { wscrSemicolonEntityName, 5, wscrSemicolonEntityValue, 1 },
    { xcapSemicolonEntityName, 5, xcapSemicolonEntityValue, 1 },
    { xcircSemicolonEntityName, 6, xcircSemicolonEntityValue, 1 },
    { xcupSemicolonEntityName, 5, xcupSemicolonEntityValue, 1 },
    { xdtriSemicolonEntityName, 6, xdtriSemicolonEntityValue, 1 },
    { xfrSemicolonEntityName, 4, xfrSemicolonEntityValue, 1 },
    { xhArrSemicolonEntityName, 6, xhArrSemicolonEntityValue, 1 },
    { xharrSemicolonEntityName, 6, xharrSemicolonEntityValue, 1 },
    { xiSemicolonEntityName, 3, xiSemicolonEntityValue, 1 },
    { xlArrSemicolonEntityName, 6, xlArrSemicolonEntityValue, 1 },
    { xlarrSemicolonEntityName, 6, xlarrSemicolonEntityValue, 1 },
    { xmapSemicolonEntityName, 5, xmapSemicolonEntityValue, 1 },
    { xnisSemicolonEntityName, 5, xnisSemicolonEntityValue, 1 },
    { xodotSemicolonEntityName, 6, xodotSemicolonEntityValue, 1 },
    { xopfSemicolonEntityName, 5, xopfSemicolonEntityValue, 1 },
    { xoplusSemicolonEntityName, 7, xoplusSemicolonEntityValue, 1 },
    { xotimeSemicolonEntityName, 7, xotimeSemicolonEntityValue, 1 },
    { xrArrSemicolonEntityName, 6, xrArrSemicolonEntityValue, 1 },
    { xrarrSemicolonEntityName, 6, xrarrSemicolonEntityValue, 1 },
    { xscrSemicolonEntityName, 5, xscrSemicolonEntityValue, 1 },
    { xsqcupSemicolonEntityName, 7, xsqcupSemicolonEntityValue, 1 },
    { xuplusSemicolonEntityName, 7, xuplusSemicolonEntityValue, 1 },
    { xutriSemicolonEntityName, 6, xutriSemicolonEntityValue, 1 },
    { xveeSemicolonEntityName, 5, xveeSemicolonEntityValue, 1 },
    { xwedgeSemicolonEntityName, 7, xwedgeSemicolonEntityValue, 1 },
    { yacuteEntityName, 6, yacuteEntityValue, 1 },
    { yacuteSemicolonEntityName, 7, yacuteSemicolonEntityValue, 1 },
    { yacySemicolonEntityName, 5, yacySemicolonEntityValue, 1 },
    { ycircSemicolonEntityName, 6, ycircSemicolonEntityValue, 1 },
    { ycySemicolonEntityName, 4, ycySemicolonEntityValue, 1 },
    { yenEntityName, 3, yenEntityValue, 1 },
    { yenSemicolonEntityName, 4, yenSemicolonEntityValue, 1 },
    { yfrSemicolonEntityName, 4, yfrSemicolonEntityValue, 1 },
    { yicySemicolonEntityName, 5, yicySemicolonEntityValue, 1 },
    { yopfSemicolonEntityName, 5, yopfSemicolonEntityValue, 1 },
    { yscrSemicolonEntityName, 5, yscrSemicolonEntityValue, 1 },
    { yucySemicolonEntityName, 5, yucySemicolonEntityValue, 1 },
    { yumlEntityName, 4, yumlEntityValue, 1 },
    { yumlSemicolonEntityName, 5, yumlSemicolonEntityValue, 1 },
    { zacuteSemicolonEntityName, 7, zacuteSemicolonEntityValue, 1 },
    { zcaronSemicolonEntityName, 7, zcaronSemicolonEntityValue, 1 },
    { zcySemicolonEntityName, 4, zcySemicolonEntityValue, 1 },
    { zdotSemicolonEntityName, 5, zdotSemicolonEntityValue, 1 },
    { zeetrfSemicolonEntityName, 7, zeetrfSemicolonEntityValue, 1 },
    { zetaSemicolonEntityName, 5, zetaSemicolonEntityValue, 1 },
    { zfrSemicolonEntityName, 4, zfrSemicolonEntityValue, 1 },
    { zhcySemicolonEntityName, 5, zhcySemicolonEntityValue, 1 },
    { zigrarrSemicolonEntityName, 8, zigrarrSemicolonEntityValue, 1 },
    { zopfSemicolonEntityName, 5, zopfSemicolonEntityValue, 1 },
    { zscrSemicolonEntityName, 5, zscrSemicolonEntityValue, 1 },
    { zwjSemicolonEntityName, 4, zwjSemicolonEntityValue, 1 },
    { zwnjSemicolonEntityName, 5, zwnjSemicolonEntityValue, 1 },
};

static const
struct pchvml_entity* pchvml_character_reference_upper_case_offset[] = {
    &pchvml_character_reference_entity_table[0],
    &pchvml_character_reference_entity_table[27],
    &pchvml_character_reference_entity_table[39],
    &pchvml_character_reference_entity_table[75],
    &pchvml_character_reference_entity_table[129],
    &pchvml_character_reference_entity_table[159],
    &pchvml_character_reference_entity_table[167],
    &pchvml_character_reference_entity_table[189],
    &pchvml_character_reference_entity_table[201],
    &pchvml_character_reference_entity_table[230],
    &pchvml_character_reference_entity_table[237],
    &pchvml_character_reference_entity_table[245],
    &pchvml_character_reference_entity_table[305],
    &pchvml_character_reference_entity_table[314],
    &pchvml_character_reference_entity_table[386],
    &pchvml_character_reference_entity_table[415],
    &pchvml_character_reference_entity_table[434],
    &pchvml_character_reference_entity_table[439],
    &pchvml_character_reference_entity_table[484],
    &pchvml_character_reference_entity_table[524],
    &pchvml_character_reference_entity_table[547],
    &pchvml_character_reference_entity_table[587],
    &pchvml_character_reference_entity_table[604],
    &pchvml_character_reference_entity_table[609],
    &pchvml_character_reference_entity_table[613],
    &pchvml_character_reference_entity_table[624],
    &pchvml_character_reference_entity_table[634],
};

static const
struct pchvml_entity* pchvml_character_reference_lower_case_offset[] = {
    &pchvml_character_reference_entity_table[634],
    &pchvml_character_reference_entity_table[703],
    &pchvml_character_reference_entity_table[819],
    &pchvml_character_reference_entity_table[918],
    &pchvml_character_reference_entity_table[984],
    &pchvml_character_reference_entity_table[1051],
    &pchvml_character_reference_entity_table[1090],
    &pchvml_character_reference_entity_table[1150],
    &pchvml_character_reference_entity_table[1178],
    &pchvml_character_reference_entity_table[1234],
    &pchvml_character_reference_entity_table[1242],
    &pchvml_character_reference_entity_table[1252],
    &pchvml_character_reference_entity_table[1406],
    &pchvml_character_reference_entity_table[1446],
    &pchvml_character_reference_entity_table[1614],
    &pchvml_character_reference_entity_table[1675],
    &pchvml_character_reference_entity_table[1744],
    &pchvml_character_reference_entity_table[1755],
    &pchvml_character_reference_entity_table[1859],
    &pchvml_character_reference_entity_table[2017],
    &pchvml_character_reference_entity_table[2075],
    &pchvml_character_reference_entity_table[2127],
    &pchvml_character_reference_entity_table[2169],
    &pchvml_character_reference_entity_table[2180],
    &pchvml_character_reference_entity_table[2204],
    &pchvml_character_reference_entity_table[2218],
    &pchvml_character_reference_entity_table[2231],
};

const struct pchvml_entity* pchvml_character_reference_first(void)
{
    return &pchvml_character_reference_entity_table[0];
}

const struct pchvml_entity* pchvml_character_reference_last(void)
{
    return &pchvml_character_reference_entity_table[2231 - 1];
}

const struct pchvml_entity* pchvml_character_reference_first_starting_with(
        char c)
{
    if (c >= 'A' && c <= 'Z') {
        return pchvml_character_reference_upper_case_offset[c - 'A'];
    }
    if (c >= 'a' && c <= 'z') {
        return pchvml_character_reference_lower_case_offset[c - 'a'];
    }
    return NULL;
}

const struct pchvml_entity* pchvml_character_reference_last_starting_with(
        char c)
{
    if (c >= 'A' && c <= 'Z') {
        return pchvml_character_reference_upper_case_offset[c - 'A' + 1] - 1;
    }
    if (c >= 'a' && c <= 'z') {
        return pchvml_character_reference_lower_case_offset[c - 'a' + 1] - 1;
    }
    return NULL;
}

/*****************************************************************************************************************
* Copyright by BSH Bosch und Siemens Hausgeraete GmbH - EDS                                                       
*                                                                                                                 
*-----------------------------------------------------------------------------------------------------------------
*                                                                                                                 
* Use, reproduction and dissemination of this software or parts of it, is not permitted without express           
* written authority of BSH EDS. The user is allowed to use this software exclusively for the development          
* of electronic boards for BSH. Usage for other purposes is strictly prohibited (e.g. development of              
* electronic boards for third parties). All rights, including copyright, rights created by patent grant           
* or registration, and rights by protection of utility patents, are reserved. Violations will be                  
* prosecuted by civil and criminal law.                                                                           
*                                                                                                                 
* The software was created and approved by acknowledged rules of technology. Because of the complexity            
* of embedded controller software the user of this software has to perform his own tests to ensure                
* proper functionality in his environment. The software was developed for usage as a software library.            
* The user is sole responsible for every other usage. The user may modify the software for adaptations            
* needed for other microcontrollers, as well as to other compilers at own risk. BSH will not support              
* any adaptations or changes. BSH is authorised to use all adaptations for own purposes free of charge.           
* BSH assumes no liability for the functionality or reliability of the software even in concrete                  
* applications. BSH assumes no liability for consequential damages, except in case of intention or gross          
* negligence. In any case the liability is limited to the typical and predictable damage.                         
*                                                                                                                 
* Technical changes are reserved.                                                                                 
*                                                                                                                 
******************************************************************************************************************
*   %PCMS_HEADER_SUBSTITUTION_START:%
*   PROJECT          CPM_DAL
*   MODULE-PREFIX    MODULE_PREFIX
*   FILE             %PM:%
*   ARCHIVE          %PP:%:%PI:%
*   PROCESSOR        independent of processor
*   COMPILER         independent of compiler 
*****************************************************************************************************************
*   AUTHOR           hallingerS
*   CREATED          11.09.2017  12:45:33         
*                                                                                                                
*   LAST CHANGE      %PRT:%
*   PRESENT REVISION %PR:%
*   STATUS           %PS:%
*****************************************************************************************************************
*   DESCRIPTION
*   Public definitions and declarations for module DAL_FFT_CALC.
*****************************************************************************************************************
*   CHANGES
*   PL:%  ( substitution of revision history switched off )
*   %PCMS_HEADER_SUBSTITUTION_END:%
*****************************************************************************************************************/


/**************************************************************************************************/
/* DOCUMENTATION                                                                                  */
/**************************************************************************************************/
/*! \file
     Public definitions and declarations for module DAL_FFT_CALC.
 
*/
#ifndef __INTEGER_FFT_H
#define __INTEGER_FFT_H

/**************************************************************************************************/
/* INCLUDES                                                                                       */
/**************************************************************************************************/

#include "LibTypes.h"


/**************************************************************************************************/
/* PUBLIC DEFINITIONS                                                                             */
/**************************************************************************************************/



/**************************************************************************************************/
/* PUBLIC TYPE DEFINITIONS                                                                         */
/**************************************************************************************************/
#ifndef fixed
#define fixed int16
#endif
/**************************************************************************************************/
/* GLOBAL VARIABLE DECLARATIONS                                                                   */
/**************************************************************************************************/
extern int fix_fft(fixed fr[], fixed fi[], int m, int inverse);
extern void window(fixed fr[], int n);
/*~T*/
/**************************************************************************************************/
/* TEST INSTRUMENTATION: to be placed after (type) definitions and before function declarations   */
/**************************************************************************************************/
/*~T*/
/**************************************************************************************************/
/* GLOBAL FUNCTION DECLARATIONS                                                                    */
/**************************************************************************************************/
/*~T*/
// __INTEGER_FFT_H
#endif

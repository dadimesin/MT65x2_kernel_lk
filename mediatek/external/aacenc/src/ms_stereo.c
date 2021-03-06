/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to MediaTek Inc. and/or its licensors. Without
 * the prior written permission of MediaTek inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of MediaTek Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 * 
 * MediaTek Inc. (C) 2010. All rights reserved.
 * 
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
 * ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES
 * TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
 * SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE
 * RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE
 * MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
 * CHARGE PAID BY RECEIVER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */

/*
 ** Copyright 2003-2010, VisualOn, Inc.
 **
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 **
 **     http://www.apache.org/licenses/LICENSE-2.0
 **
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 */
/*******************************************************************************
	File:		ms_stereo.c

	Content:	MS stereo processing function

*******************************************************************************/

#include "basic_op.h"
#include "oper_32b.h"
#include "psy_const.h"
#include "ms_stereo.h"


/********************************************************************************
*
* function name: MsStereoProcessing
* description:  detect use ms stereo or not
*				if ((min(thrLn, thrRn)*min(thrLn, thrRn))/(enMn*enSn))
*				>= ((thrLn *thrRn)/(enLn*enRn)) then ms stereo
*
**********************************************************************************/
void MsStereoProcessing(Word32       *sfbEnergyLeft,
                        Word32       *sfbEnergyRight,
                        const Word32 *sfbEnergyMid,
                        const Word32 *sfbEnergySide,
                        Word32       *mdctSpectrumLeft,
                        Word32       *mdctSpectrumRight,
                        Word32       *sfbThresholdLeft,
                        Word32       *sfbThresholdRight,
                        Word32       *sfbSpreadedEnLeft,
                        Word32       *sfbSpreadedEnRight,
                        Word16       *msDigest,
                        Word16       *msMask,
                        const Word16  sfbCnt,
                        const Word16  sfbPerGroup,
                        const Word16  maxSfbPerGroup,
                        const Word16 *sfbOffset) {
  Word32 temp;
  Word32 sfb,sfboffs, j;
  Word32 msMaskTrueSomewhere = 0;
  Word32 msMaskFalseSomewhere = 0;

  for (sfb=0; sfb<sfbCnt; sfb+=sfbPerGroup) {
    for (sfboffs=0;sfboffs<maxSfbPerGroup;sfboffs++) {

      Word32 temp;
      Word32 pnlr,pnms;
      Word32 minThreshold;
      Word32 thrL, thrR, nrgL, nrgR;
      Word32 idx, shift;

      idx = sfb + sfboffs;

      thrL = sfbThresholdLeft[idx];
      thrR = sfbThresholdRight[idx];
      nrgL = sfbEnergyLeft[idx];
      nrgR = sfbEnergyRight[idx];

      minThreshold = min(thrL, thrR);

      nrgL = max(nrgL,thrL) + 1;
      shift = norm_l(nrgL);
	  nrgL = Div_32(thrL << shift, nrgL << shift);
      nrgR = max(nrgR,thrR) + 1;
      shift = norm_l(nrgR);
	  nrgR = Div_32(thrR << shift, nrgR << shift);

	  pnlr = fixmul(nrgL, nrgR);

      nrgL = sfbEnergyMid[idx];
      nrgR = sfbEnergySide[idx];

      nrgL = max(nrgL,minThreshold) + 1;
      shift = norm_l(nrgL);
	  nrgL = Div_32(minThreshold << shift, nrgL << shift);

      nrgR = max(nrgR,minThreshold) + 1;
      shift = norm_l(nrgR);
	  nrgR = Div_32(minThreshold << shift, nrgR << shift);

      pnms = fixmul(nrgL, nrgR);

      temp = (pnlr + 1) / ((pnms >> 8) + 1);

      temp = pnms - pnlr;
      if( temp > 0 ){

        msMask[idx] = 1;
        msMaskTrueSomewhere = 1;

        for (j=sfbOffset[idx]; j<sfbOffset[idx+1]; j++) {
          Word32 left, right;
          left  = (mdctSpectrumLeft[j] >>  1);
          right = (mdctSpectrumRight[j] >> 1);
          mdctSpectrumLeft[j] =  left + right;
          mdctSpectrumRight[j] =  left - right;
        }

        sfbThresholdLeft[idx] = minThreshold;
        sfbThresholdRight[idx] = minThreshold;
        sfbEnergyLeft[idx] = sfbEnergyMid[idx];
        sfbEnergyRight[idx] = sfbEnergySide[idx];

        sfbSpreadedEnRight[idx] = min(sfbSpreadedEnLeft[idx],sfbSpreadedEnRight[idx]) >> 1;
        sfbSpreadedEnLeft[idx] = sfbSpreadedEnRight[idx];

      }
      else {
        msMask[idx]  = 0;
        msMaskFalseSomewhere = 1;
      }
    }
    if ( msMaskTrueSomewhere ) {
      if(msMaskFalseSomewhere ) {
        *msDigest = SI_MS_MASK_SOME;
      } else {
        *msDigest = SI_MS_MASK_ALL;
      }
    } else {
      *msDigest = SI_MS_MASK_NONE;
    }
  }

}

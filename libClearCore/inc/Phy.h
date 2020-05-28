/*
* Copyright (c) 2020 Teknic, Inc.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/

/**
    \file Phy.h
    An incomplete interface to the KSZ8081 Ethernet PHY chip.
**/
/************************ PHY ****************************/
#ifndef __PHY_H__
#define __PHY_H__

#ifndef HIDE_FROM_DOXYGEN
// Ethernet PHY Operations
#define PHY_READ_OP         2
#define PHY_WRITE_OP        1

// Ethernet PHY Auto-Negotiated Modes
#define PHY_CTRL_AN_MSK 0x0007
#define BASE_10_HDX     0x1
#define BASE_10_FDX     0x5
#define BASE_100_HDX    0x2
#define BASE_100_FDX    0x6

#define PHY_CTRL_AN_SPD_MSK 0x2
#define PHY_CTRL_AN_FD_MSK  0x4

// Ethernet PHY Registers
#define PHY_B_CTRL      0x00
#define PHY_B_STATUS    0x01
#define PHY_ID_1        0x02
#define PHY_ID_2        0x03
#define PHY_ANA         0x04
#define PHY_ANLP        0x05
#define PHY_ANE         0x06
#define PHY_ANNP        0x07
#define PHY_LPNP        0x08
// reserved
#define PHY_DRC         0x10
#define PHY_AFE_CTRL    0x11
// reserved
#define PHY_RXER_COUNT  0x15
#define PHY_ICS         0x1B
#define PHY_LMD         0x1D
#define PHY_CTRL_1      0x1E
#define PHY_CTRL_2      0x1F

// PHY_B_CTRL: Ethernet PHY Register 0h - Basic Control
#define PHY_B_CTRL_COLEN_Pos 7   /**< \brief (PHY_B_CTRL) Enable Collision Test */
#define PHY_B_CTRL_COLEN     (_U_(0x1) << PHY_B_CTRL_COLEN_Pos)
#define PHY_B_CTRL_DPLX_Pos  8   /**< \brief (PHY_B_CTRL) Duplex Mode */
#define PHY_B_CTRL_DPLX      (_U_(0x1) << PHY_B_CTRL_DPLX_Pos)
#define PHY_B_CTRL_RAN_Pos   9   /**< \brief (PHY_B_CTRL) Restart Auto-Negotiation */
#define PHY_B_CTRL_RAN       (_U_(0x1) << PHY_B_CTRL_RAN_Pos)
#define PHY_B_CTRL_ISO_Pos   10  /**< \brief (PHY_B_CTRL) Electrical Isolation of PHY */
#define PHY_B_CTRL_ISO       (_U_(0x1) << PHY_B_CTRL_ISO_Pos)
#define PHY_B_CTRL_PD_Pos    11  /**< \brief (PHY_B_CTRL) Power-Down */
#define PHY_B_CTRL_PD        (_U_(0x1) << PHY_B_CTRL_PD_Pos)
#define PHY_B_CTRL_ANEN_Pos  12  /**< \brief (PHY_B_CTRL) Auto-Negotiation Enable */
#define PHY_B_CTRL_ANEN      (_U_(0x1) << PHY_B_CTRL_ANEN_Pos)
#define PHY_B_CTRL_SPD_Pos   13  /**< \brief (PHY_B_CTRL) Speed Select */
#define PHY_B_CTRL_SPD       (_U_(0x1) << PHY_B_CTRL_SPD_Pos)
#define PHY_B_CTRL_LB_Pos    14  /**< \brief (PHY_B_CTRL) Loopback Mode */
#define PHY_B_CTRL_LB        (_U_(0x1) << PHY_B_CTRL_LB_Pos)
#define PHY_B_CTRL_RES_Pos   15  /**< \brief (PHY_B_CTRL) Software Reset */
#define PHY_B_CTRL_RES       (_U_(0x1) << PHY_B_CTRL_RES_Pos)

// PHY_B_STATUS: Ethernet PHY Register 1h - Basic Status
#define PHY_B_STATUS_EC_Pos     0   /**< \brief (PHY_B_STATUS) Extended Capability */
#define PHY_B_STATUS_EC         (_U_(0x1) << PHY_B_STATUS_EC_Pos)
#define PHY_B_STATUS_JAB_Pos    1   /**< \brief (PHY_B_STATUS) Jabber Detect */
#define PHY_B_STATUS_JAB        (_U_(0x1) << PHY_B_STATUS_JAB_Pos)
#define PHY_B_STATUS_LU_Pos     2   /**< \brief (PHY_B_STATUS) Link Status */
#define PHY_B_STATUS_LU         (_U_(0x1) << PHY_B_STATUS_LU_Pos)
#define PHY_B_STATUS_AN_Pos     3   /**< \brief (PHY_B_STATUS) Auto-Negotiation Ability */
#define PHY_B_STATUS_AN         (_U_(0x1) << PHY_B_STATUS_AN_Pos)
#define PHY_B_STATUS_REMF_Pos   4   /**< \brief (PHY_B_STATUS) Remote Fault */
#define PHY_B_STATUS_REMF       (_U_(0x1) << PHY_B_STATUS_REMF_Pos)
#define PHY_B_STATUS_ANC_Pos    5   /**< \brief (PHY_B_STATUS) Auto-Negotiation Complete */
#define PHY_B_STATUS_ANC        (_U_(0x1) << PHY_B_STATUS_ANC_Pos)
#define PHY_B_STATUS_NPRE_Pos   6   /**< \brief (PHY_B_STATUS) No Preamble */
#define PHY_B_STATUS_NPRE       (_U_(0x1) << PHY_B_STATUS_NPRE_Pos)
#define PHY_B_STATUS_TBTHD_Pos  11  /**< \brief (PHY_B_STATUS) 10BASE-T Half-Duplex Capable */
#define PHY_B_STATUS_TBTHD      (_U_(0x1) << PHY_B_STATUS_TBTHD_Pos)
#define PHY_B_STATUS_TBTFD_Pos  12  /**< \brief (PHY_B_STATUS) 10BASE-T Full-Duplex Capable */
#define PHY_B_STATUS_TBTFD      (_U_(0x1) << PHY_B_STATUS_TBTFD_Pos)
#define PHY_B_STATUS_HBTHD_Pos  13  /**< \brief (PHY_B_STATUS) 100BASE-T Half-Duplex Capable */
#define PHY_B_STATUS_HBTHD      (_U_(0x1) << PHY_B_STATUS_HBTHD_Pos)
#define PHY_B_STATUS_HBTFD_Pos  14  /**< \brief (PHY_B_STATUS) 100BASE-T Full-Duplex Capable */
#define PHY_B_STATUS_HBTFD      (_U_(0x1) << PHY_B_STATUS_HBTFD_Pos)
#define PHY_B_STATUS_TFOUR_Pos  15  /**< \brief (PHY_B_STATUS) 100BASE-T4 Capable */
#define PHY_B_STATUS_TFOUR      (_U_(0x1) << PHY_B_STATUS_TFOUR_Pos)

// PHY_ICS : Ethernet PHY Register 1Bh - Interrupt Control/Status
#define PHY_ICS_LU_Pos      0   /**< \brief (PHY_ICS) Link-Up Interrupt */
#define PHY_ICS_LU          (_U_(0x1) << PHY_ICS_LU_Pos)
#define PHY_ICS_RF_Pos      1   /**< \brief (PHY_ICS) Remote Fault Interrupt */
#define PHY_ICS_RF          (_U_(0x1) << PHY_ICS_RF_Pos)
#define PHY_ICS_LD_Pos      2   /**< \brief (PHY_ICS) Link-Down Interrupt */
#define PHY_ICS_LD          (_U_(0x1) << PHY_ICS_LD_Pos)
#define PHY_ICS_LPA_Pos     3   /**< \brief (PHY_ICS) Link Partner Acknowledge Interrupt */
#define PHY_ICS_LPA         (_U_(0x1) << PHY_ICS_LPAEN_Pos)
#define PHY_ICS_PDF_Pos     4   /**< \brief (PHY_ICS) Parallel Detect Fault Interrupt */
#define PHY_ICS_PDF         (_U_(0x1) << PHY_ICS_PDF_Pos)
#define PHY_ICS_PRX_Pos     5   /**< \brief (PHY_ICS) Page Receive Interrupt */
#define PHY_ICS_PRX         (_U_(0x1) << PHY_ICS_PRX_Pos)
#define PHY_ICS_RXER_Pos    6   /**< \brief (PHY_ICS) Receive Error Interrupt */
#define PHY_ICS_RXER        (_U_(0x1) << PHY_ICS_RXER_Pos)
#define PHY_ICS_JAB_Pos     7   /**< \brief (PHY_ICS) Jabber Interrupt */
#define PHY_ICS_JAB         (_U_(0x1) << PHY_ICS_JAB_Pos)
#define PHY_ICS_LUEN_Pos    8   /**< \brief (PHY_ICS) Link-Up Interrupt Enable */
#define PHY_ICS_LUEN        (_U_(0x1) << PHY_ICS_LUEN_Pos)
#define PHY_ICS_RFEN_Pos    9   /**< \brief (PHY_ICS) Remote Fault Interrupt Enable */
#define PHY_ICS_RFEN        (_U_(0x1) << PHY_ICS_RFEN_Pos)
#define PHY_ICS_LDEN_Pos    10  /**< \brief (PHY_ICS) Link-Down Interrupt Enable */
#define PHY_ICS_LDEN        (_U_(0x1) << PHY_ICS_LDEN_Pos)
#define PHY_ICS_LPAEN_Pos   11  /**< \brief (PHY_ICS) Link Partner Acknowledge Interrupt Enable */
#define PHY_ICS_LPAEN       (_U_(0x1) << PHY_ICS_LPAEN_Pos)
#define PHY_ICS_PDFEN_Pos   12  /**< \brief (PHY_ICS) Parallel Detect Fault Interrupt Enable */
#define PHY_ICS_PDFEN       (_U_(0x1) << PHY_ICS_PDFEN_Pos)
#define PHY_ICS_PRXEN_Pos   13  /**< \brief (PHY_ICS) Page Receive Interrupt Enable */
#define PHY_ICS_PRXEN       (_U_(0x1) << PHY_ICS_PRXEN_Pos)
#define PHY_ICS_RXEREN_Pos  14  /**< \brief (PHY_ICS) Receive Error Interrupt Enable */
#define PHY_ICS_RXEREN      (_U_(0x1) << PHY_ICS_RXEREN_Pos)
#define PHY_ICS_JABEN_Pos   15  /**< \brief (PHY_ICS) Jabber interrupt enable */
#define PHY_ICS_JABEN       (_U_(0x1) << PHY_ICS_JABEN_Pos)

// PHY_LMD : Ethernet Phy Register 1Dh - LinkMD Control/Status
#define PHY_LMD_FC_Pos      0   /**< \brief (PHY_LMD) Cable Fault Counter */
#define PHY_LMD_FC_Msk      (_U_(0x1FF) << PHY_LMD_FC_Pos)
#define PHY_LMD_FC(value)   (PHY_LMD_FC_Msk & ((value) << PHY_LMD_FC_Pos))
#define PHY_LMD_SCI_Pos     12  /**< \brief (PHY_LMD) Short Cable Indicator */
#define PHY_LMD_SCI         (_U_(0x1) << PHY_LMD_SCI_Pos)
#define PHY_LMD_CDTR_Pos    13  /**< \brief (PHY_LMD) Cable Diagnostic Test Result */
#define PHY_LMD_CDTR_Msk    (_U_(0x3) << PHY_LMD_CDTR_Pos)
#define PHY_LMD_CDTR(value) (PHY_LMD_CDTR_Msk & ((value) << PHY_LMD_CDTR_Pos))
#define PHY_LMD_CDEN_Pos    15  /**< \brief (PHY_LMD) Cable Diagnostic Test Enable */
#define PHY_LMD_CDEN        (_U_(0x1) << PHY_LMD_CDEN_Pos)

#endif // !HIDE_FROM_DOXYGEN

#endif // !__PHY_H__
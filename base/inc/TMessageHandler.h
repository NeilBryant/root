// @(#)root/base:$Name:  $:$Id: TMessageHandler.h,v 1.1.1.1 2000/05/16 17:00:39 rdm Exp $
// Author: Rene Brun   11/11/99

/*************************************************************************
 * Copyright (C) 1995-2000, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_TMessageHandler
#define ROOT_TMessageHandler


//////////////////////////////////////////////////////////////////////////
//                                                                      //
// TMessageHandler                                                      //
//                                                                      //
// Handle messages that might be generated by the system.               //
// By default a handler only keeps track of the different messages      //
// generated for a specific class. By deriving from this class and      //
// overriding Notify() one can implement custom message handling.       //
// In Notify() one has access to the message id and the object          //
// generating the message. One can install more than one message        //
// handler per class. A message handler can be removed or again         //
// added when needed.                                                   //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#ifndef ROOT_TNamed
#include "TNamed.h"
#endif

class TMessageHandler : public TNamed {

protected:
   const TClass   *fClass;      // class for which message has to be handled
   const TObject  *fMessObj;    // object generating message
   Int_t           fMessId;     // message id (often matching specific enum in fClass)
   Int_t           fSize;       // number of different messages handled
   Int_t          *fCnts;       // count per message
   Int_t          *fMessIds;    // message ids
   Bool_t          fDerived;    // if true handle messages also for derived classes

public:
   TMessageHandler(const TClass *cl, Bool_t derived = kTRUE);
   TMessageHandler(const char *cl, Bool_t derived = kTRUE);
   virtual ~TMessageHandler();

   virtual void    Add();

   Int_t           GetSize() const { return fSize; }
   virtual Int_t   GetMessageCount(Int_t messId) const;
   virtual Int_t   GetTotalMessageCount() const;
   Bool_t          HandleDerived() const { return fDerived; }
   virtual void    HandleMessage(Int_t id, const TObject *obj);

   virtual Bool_t  Notify();

   virtual void    Print(Option_t *option= "") const;
   virtual void    Remove();

   ClassDef(TMessageHandler,0)  // Generic message handler
};

#endif

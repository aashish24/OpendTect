#ifndef uiListBox_H
#define uiListBox_H

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          16/05/2000
 RCS:           $Id: uilistbox.h,v 1.29 2004-10-07 16:16:52 nanne Exp $
________________________________________________________________________

-*/

#include "uigroup.h"

class BufferStringSet;
class Color;
class ioPixmap;
class uiLabel;
class uiListBoxBody;

class QString;
class QPixmap;


class uiListBox : public uiObject
{
friend class i_listMessenger;
public:

                        uiListBox(uiParent* parnt=0, 
				  const char* nm="uiListBox",
				  bool isMultiSelect=false,
				  int preferredNrLines=0,
				  int preferredFieldWidth=0);

			uiListBox(uiParent*,const BufferStringSet&,
				  const char* txt="uiListBox",
				  bool isMultiSelect=false,
				  int preferredNrLines=0,
				  int preferredFieldWidth=0);

    virtual 		~uiListBox();

/*! \brief set preferred number of lines. 
    If set to 0, then it is determined by the number of items in list.
    If set to 1, then the list has a fixed height of 1 textline and 
    therefore should not be able to grow/shrink vertically.

    adaptVStretch specifies wether or not the vertical stretch should be
    set to 0 if nrlines == 1 or 2 otherwise.
*/
    void 		setLines(int, bool adaptVStretch);
    void		setNotSelectable();
    void		setMultiSelect(bool yn=true);

    int			size() const;
    bool		isPresent(const char*) const;
    bool		isSelected(int) const;
    int			nrSelected() const;
    void		setSelected(int,bool yn=true);
    void		selAll(bool yn=true);
    virtual void	clear();
    void		sort(bool asc=true);

    void		empty();
    void		removeItem(int);
    void		addItem(const char*,bool embedded=false); 
    			//!< embedded = put [...] around text
    void		addItems(const char**); 
    void		addItems(const BufferStringSet&);
    void		insertItem(const char*,int idx=-1,bool embedded=false);
    void		insertItem(const char*,const Color&,int idx=-1);
    void		insertItem(const char*,const ioPixmap&,int idx=-1);
    void		setColor(const Color&,int);
    ioPixmap*		pixmap(int);

    void		setItemText(int,const char*);
    int			currentItem() const;
    const char*		getText() const	 { return textOfItem(currentItem()); }
    const char*		textOfItem(int,bool disembed=false) const;
    			//!< disembed = remove [...] from text
    bool		isEmbedded(int) const;
    			//!< check for [...] around text
    void                setCurrentItem(int);
    void                setCurrentItem(const char*); //!< First match

    void		getSelectedItems(BufferStringSet&);
    void		getSelectedItems(TypeSet<int>&);

    void		setFieldWidth(int);
    int			optimumFieldWidth(int minwdth=20,int maxwdth=40) const;

    int			lastClicked()		{ return lastClicked_; }

    Notifier<uiListBox> selectionChanged;

			//! sets lastClicked
    Notifier<uiListBox> doubleClicked;

			//! sets lastClicked
    Notifier<uiListBox> rightButtonClicked;

protected:

    mutable BufferString	rettxt;
    int				lastClicked_;

private:

    uiListBoxBody*	body_;
    uiListBoxBody&	mkbody(uiParent*,const char*,bool,int,int);

    void		createQString(QString&,const char*,bool);
    void		createQPixmap(QPixmap&,int);
};


class uiLabeledListBox : public uiGroup
{
public:

    enum LblPos	{ LeftTop, RightTop,
		  AboveLeft, AboveMid, AboveRight,
		  BelowLeft, BelowMid, BelowRight };

		uiLabeledListBox( uiParent*,const char* txt,
				  bool multisel=false,LblPos p=LeftTop);
		uiLabeledListBox( uiParent*,const BufferStringSet&,
				  const char* txt,
				  bool multisel=false,LblPos p=LeftTop);

    uiListBox*	box()				{ return lb; }

    int		nrLabels() const		{ return lbls.size(); }
    uiLabel*	label( int nr=0 )		{ return lbls[nr]; }
    const char*	labelText(int nr=0) const;
    void	setLabelText(const char*,int nr=0);


protected:

    uiListBox*		lb;
    ObjectSet<uiLabel>	lbls;

    void		mkRest(const char*,LblPos);

};


#endif

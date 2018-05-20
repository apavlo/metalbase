#
# This is form sample--designed as a demonstration data-entry form.  It
# must be compiled with 'form'; sample_fm.h (built from this by form) is used
# in sample.c to control data-entry.
#
# Released October 1st, 1992 by Huan-Ti [ virtual!root@owlnet.rice.edu ]
#                                       [ t-richj@microsoft.com ]
#

Data sample;

Define A credit;    # These names are too long for their DE slots,
Define B temp;      # So they're defined to something shorter.
Define C num

#
# Oh yeah.  I added the line below for Number Purchased on a whim... you'll
# find no mention of it in sample.c; but it works just perfectly, data moving
# from data-entry template to the relation and back without any specific
# coding at all.  Good example of some hefty flexibility here...
#
# The "4 2" below are the Y,X coordinates of the upper-left corner of this
# data-entry template on the screen.  Just thought I'd mention that.
#

Screen 2 2
{
     +--------------------------------------------------------------+
     |                                                              |
     |  Customer Number...[custnum]   Name...[custname           ]  |
     |  Customer Phone....[phone               ]                    |
     |                                                              |
     |  Current Balance...${balance  }  Accept Credit...[A]..[B    ]  |
     |  Number Purchased..[C  ]                                     |
     |                                                              |
     |  Date Entered......[date_en   ]  Time...[time_en ]           |
     |                                                              |
     +--------------------------------------------------------------+

            Ctrl-U          -  Undo (in this field only)
            Ctrl-Q, Ctrl-C  -  Abort a change
            Ctrl-A, Ctrl-D  -  Accept transaction; Delete this record
            Ctrl-N, Ctrl-P  -  Find next record; Find previous record

      Type CTRL-Q (to leave edit mode), then "new" as a customer name
            to add a new record.
}

#
# Note the screwed-up right side on the template above.  The braces
# surrounding "balance" will be actually -removed- from the display, while
# the brackets around the others are simply spaced over.  So the template
# looks perfectly rectangular during data-entry.
#
# I only used braces because I wanted the number for the customer's balance
# to buck right up against the dollar sign.  I know--picky picky...
#

Field credit type choice ("Yy" "Nn" "?");
Field temp   type link to credit ("Yes" "No" "Maybe");

#
# Ah.  The two lines above create a choice-and-link pair... something I
# noticed I'd been writing a lot of hard-coded a while back, and decided to
# support.  Without the "Field temp type link to credit..." thing, the
# field "sample.credit" would continue to work as you'd expect from a choice
# field--it only allows characters from the set "YyNn?" in it.  But, add the
# link to credit, and field "temp" is filled in automatically depending on
# what "sample.credit" is:  "Yy" maps to "Yes", "Nn" to "No", and "?" to
# "Maybe".
#
# Note that, an alternate setup (if the relation had sample.credit defined
# to have a few more characters) might be:
#
# Field temp   type choice ("Yy" "Nn" "?");
# Field credit type link to temp ("Yes" "No" "Maybe");
#
#    Accept Credit?....[temp]..[credit ]
#
# That way, the Yes/No/Maybe is stored in the relation, instead of Y/N/?.
# Nifty, eh?  Links can only be made to choice fields, and choices need not
# have links.  A field can't be both a link and a choice, nor can a single
# field be a link to more than one choice field... but a choice field can
# have multiple links.  Confused yet?  Play with it.
#

Mode 1 inout  custnum out,   temp out, date_en out, time_en  out; # Add
Mode 2 out    custnum inout, custname inout;                      # Query
Mode 3 inout                 temp out, date_en out, time_en  out; # Update

#
# Mode 1 is for adding new records,
# Mode 2 is for finding old records,
# Mode 3 is for updating existing records.
#

End


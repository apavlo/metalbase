relation sample

#
#  Each customer at a store, for example, has one record in this relation--the
#  record contains their name and a customer code (assigned by the system, with
#  the first customer being 100), their balance and number of purchases made,
#  the date and time the record was established, and a 3-character string that
#  describes whether or not they're allowed credit at the store.
#

field custname type string length 30; # A fairly short name field, but hey...
field custnum  type serial start 100; # Assigned automatically (from 100)
field balance  type money;            # Standard double, to .XX resolution
field date_en  date;                  # Date customer entered into database
field time_en  type time;             # Time customer entered into database
field credit   char * 3;              # "y"/"n"/"?" etc.
field num      ushort;                # Number of purchases customer has made
field phone    type phone;            # Phone number of client

index ix_name    on custname with duplicates;
index ix_number  on custnum;
index ix_balance on balance            with duplicates;
index ix_entered on date_en, time_en   with duplicates;  # DATE_EN FIRST!!!

#
# Why date_en first?  Because a composite index, like ix_entered, will
# sort merrily along the first field you give it... as soon as it finds two
# records which have the same first field, it'll sort 'em by the second.  So
# ix_entered will sort things by date, and when the date's the same, by time..
# if you did it the other way around, it'd be a pretty stupid index.
#

end


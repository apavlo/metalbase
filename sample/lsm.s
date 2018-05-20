relation lsm

field pkgname	type string 36;
field title	type string 36;
field version	type string 25;
field desc	type string 67; 
field desc2	type string 67; 
field desc3	type string 67; 
field maintby	type string 67; 
field maintat	type string 67; 
field pathfile1	type string 67; 
field mainat2	type string 67; 
field pathfile2	type string 67; 
field platform1	type string 67; 
field platform2	type string 67; 
field platform3	type string 67; 
field copypolicy	type string 67; 
field keywords	type string 67; 
field approxsize	type string 15; 
field last3rel	type string 67; 
field comment1	type string 67; 
field comment2	type string 67; 
field comment3	type string 67; 

index ix_title	on title;

end

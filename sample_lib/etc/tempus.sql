-- TABLES
create table accounts (
	idnum			integer primary key,
	name			varchar(20),
	password		varchar(100),
	email			varchar(60),
	creation_time	timestamp,
	creation_addr	varchar(60),
	login_time		timestamp,
	login_addr		varchar(60),
	entry_time		timestamp,
	ansi_level		integer,
	term_height		integer,
	term_width		integer,
	bank_past		bigint,
	bank_future		bigint,
    trust integer,
    compact_level integer,
    reputation integer,
    quest_points integer,
    quest_banned boolean,
    banned boolean,
    metric_units boolean
);

create table players (
	idnum			integer primary key,
	account			integer references accounts(idnum),
	name			varchar(20)
);

create table trusted (
	account			integer references accounts(idnum),
	player 			integer references players(idnum)
);

create table clans (
	idnum			integer primary key,
	name			varchar(40),
	badge			varchar(40),
	password		varchar(40),
	bank			bigint,
	owner			integer references players(idnum),
    flags           integer
);

create table clan_ranks (
	clan			integer references clans(idnum),
	rank			integer,
	title			varchar(48)
);

create table clan_members (
	clan			integer references clans(idnum),
	player			integer references players(idnum),
	rank			integer,
    no_mail         boolean
);

create table clan_rooms (
	clan			integer references clans(idnum),
	room			integer
);

create table sgroups (
	idnum			integer primary key,
	name			varchar(48),
	descrip			varchar(256),
	admin			integer references sgroups(idnum)
);

create table sgroup_commands (
	sgroup			integer references sgroups(idnum),
	command			varchar(20)
);

create table sgroup_members (
	sgroup			integer references sgroups(idnum),
	player			integer references players(idnum)
);

create table voting_polls (
	idnum			serial primary key,
	creation_time	timestamp,
	header			varchar(80),
	descrip			text,
	secret			boolean
);

create table voting_options (
	poll		integer references voting_polls(idnum),
	descrip		text,
	idx			integer,
	count		integer
);

create table voting_accounts (
	poll		integer references voting_polls(idnum),
	account		integer references accounts(idnum)
);

create table board_messages (
	idnum			serial primary key,
	board			varchar(20),
	post_time		timestamp,
	author			integer references players(idnum),
	name			varchar(80),
	subject			varchar(80),
	body			text
);
create table bounty_hunters (
    idnum           integer,
    victim          integer
);
create table christmas_quest (
    idnum           integer
);

-- INDEXES
create index accounts_name_idx on accounts (lower(name));
create index players_account on players (account);
create index players_name_idx on players (lower(name));
create index board_messages_board_idx on board_messages (board);

-- SEQUENCES
create sequence unique_id
    start with 1
    increment by 1
    no maxvalue
    no minvalue
    cache 1;

-- DATA
insert into sgroups values
    (1,'AdminBasic','Basic mortal administration commands',2),
    (2,'AdminFull','All mortal administration commands',19),
    (3,'Clan','Clan editing commands',4),
    (4,'ClanAdmin','Group administration for Clan group',34),
    (5,'Coder','Coder commands',6),
    (6,'CoderAdmin','Coders administration group',),
    (7,'Dynedit','The Dynamic Text System',5),
    (8,'GroupsAdmin','Global group administration',),
    (9,'Help','Help system editing',21),
    (10,'House','Personal house construction capability',34),
    (11,'OLC','Basic OLC building capability',12),
    (12,'OLCAdmin','Group administration for OLC groups',34),
    (13,'OLCApproval','Approval capability',12),
    (14,'OLCProofer','Proofing capability ( Edit !approved zones )',12),
    (15,'OLCWorldWrite','World write access',34),
    (16,'Questor','Basic questor commands',17),
    (17,'QuestorAdmin','Group administration for Questors',8),
    (18,'Testers','Tester Characters',12),
    (19,'WizardAdmin','Group administration for Wizard groups',8),
    (20,'WizardBasic','Basic Wizard commands',19),
    (21,'WizardFull','Full Wizard commands',19),
    (27,'Wizlist_Blders','The Builders wizlist section',12),
    (28,'Wizlist_Coders','The Coders wizlist section',6),
    (29,'Wizlist_Elders','Old immortals who no longer play',8),
    (31,'Wizlist_Founders','Founders of TempusMUD',),
    (33,'Wizlist_Quests','The Questors wizlist section',17),
    (34,'WorldAdmin','Groups administration for world groups',8),
    (35,'Wizlist_Architects','The architects wizlist section',);

insert into sgroup_commands values
    (1,'ban'),
    (1,'transport'),
    (1,'notitle'),
    (1,'nopost'),
    (1,'snoop'),
    (1,'users'),
    (2,'dc'),
    (2,'pload'),
    (2,'purge'),
    (2,'unban'),
    (3,'cedit'),
    (5,'slowns'),
    (5,'logdeaths'),
    (5,'map'),
    (5,'qpreload'),
    (5,'reload'),
    (5,'shutdow'),
    (5,'shutdown'),
    (5,'change_weather'),
    (5,'wizlock'),
    (5,'coderutil'),
    (7,'dynedit'),
    (9,'hcollection'),
    (10,'hcontrol'),
    (11,'nowho'),
    (11,'bomb'),
    (11,'install'),
    (11,'olc'),
    (11,'olchelp'),
    (11,'rstat'),
    (11,'uninstall'),
    (11,'vnum'),
    (11,'vstat'),
    (11,'rlist'),
    (11,'olist'),
    (11,'mlist'),
    (11,'xlist'),
    (13,'approve'),
    (13,'unapprove'),
    (16,'addname'),
    (16,'qcontrol'),
    (16,'qecho'),
    (16,'qlog'),
    (16,'rename'),
    (17,'freeze'),
    (17,'mute'),
    (17,'restore'),
    (17,'thaw'),
    (18,'tester'),
    (20,'addname'),
    (20,'distance'),
    (20,'echo'),
    (20,'force'),
    (20,'gecho'),
    (20,'nolocate'),
    (20,'nowiz'),
    (20,'oecho'),
    (20,'peace'),
    (20,'rename'),
    (20,'set'),
    (20,'switch'),
    (20,'teleport'),
    (20,'users'),
    (20,'wiznet'),
    (20,';'),
    (20,'wizlick'),
    (20,'wizflex'),
    (20,'hcollection'),
    (21,'addpos'),
    (21,'advance'),
    (21,'logall'),
    (21,'makemount'),
    (21,'mload'),
    (21,'oload'),
    (21,'oset'),
    (21,'pload'),
    (21,'reload'),
    (21,'reroll'),
    (21,'restore'),
    (21,'rswitch'),
    (21,'send'),
    (21,'skset'),
    (21,'skillset'),
    (21,'unaffect'),
    (21,'xlag'),
    (5,'logall'),
    (21,'zreset'),
    (2,'account'),
    (17,'qpreload'),
    (21,'edit'),
    (2,'edit'),
    (11,'purge'),
    (1,'zoneecho'),
    (1,'gecho'),
    (1,'echo'),
    (2,'freeze'),
    (2,'thaw'),
    (2,'mute'),
    (5,'handbook'),
    (1,'ban'),
    (1,'transport'),
    (1,'notitle'),
    (1,'nopost'),
    (1,'snoop'),
    (1,'users'),
    (2,'dc'),
    (2,'pload'),
    (2,'purge'),
    (2,'unban'),
    (3,'cedit'),
    (5,'slowns'),
    (5,'logdeaths'),
    (5,'map'),
    (5,'qpreload'),
    (5,'reload'),
    (5,'shutdow'),
    (5,'shutdown'),
    (5,'change_weather'),
    (5,'wizlock'),
    (5,'coderutil'),
    (7,'dynedit'),
    (9,'hcollection'),
    (10,'hcontrol'),
    (11,'nowho'),
    (11,'bomb'),
    (11,'install'),
    (11,'olc'),
    (11,'olchelp'),
    (11,'rstat'),
    (11,'uninstall'),
    (11,'vnum'),
    (11,'vstat'),
    (11,'rlist'),
    (11,'olist'),
    (11,'mlist'),
    (11,'xlist'),
    (13,'approve'),
    (13,'unapprove'),
    (16,'addname'),
    (16,'qcontrol'),
    (16,'qecho'),
    (16,'qlog'),
    (16,'rename'),
    (17,'freeze'),
    (17,'mute'),
    (17,'restore'),
    (17,'thaw'),
    (18,'tester'),
    (20,'addname'),
    (20,'distance'),
    (20,'echo'),
    (20,'force'),
    (20,'gecho'),
    (20,'nolocate'),
    (20,'nowiz'),
    (20,'oecho'),
    (20,'peace'),
    (20,'rename'),
    (20,'set'),
    (20,'switch'),
    (20,'teleport'),
    (20,'users'),
    (20,'wiznet'),
    (20,';'),
    (20,'wizlick'),
    (20,'wizflex'),
    (20,'hcollection'),
    (21,'addpos'),
    (21,'advance'),
    (21,'logall'),
    (21,'makemount'),
    (21,'mload'),
    (21,'oload'),
    (21,'oset'),
    (21,'pload'),
    (21,'reload'),
    (21,'reroll'),
    (21,'restore'),
    (21,'rswitch'),
    (21,'send'),
    (21,'skset'),
    (21,'skillset'),
    (21,'unaffect'),
    (21,'xlag'),
    (5,'logall'),
    (21,'zreset'),
    (2,'account'),
    (17,'qpreload'),
    (21,'edit'),
    (2,'edit'),
    (11,'purge'),
    (1,'zoneecho'),
    (1,'gecho'),
    (1,'echo'),
    (2,'freeze'),
    (2,'thaw'),
    (2,'mute'),
    (5,'handbook');

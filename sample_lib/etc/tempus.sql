-- TABLES
create table accounts (
	idnum			integer primary key,
	name			varchar(20),
	password		varchar(40),
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
	bank_future		bigint
);

create table skills (
	idnum			integer primary key,
	name			varchar(40),
	wear_off_msg	varchar(80)
);

create table classes (
	idnum			integer primary key,
	name			varchar(40),
	abbrev			varchar(4),
	color			char,
	pc				boolean,
	mort_learned	integer,
	min_gain		integer,
	max_gain		integer,
	prac_type		varchar(10),
	is_past			boolean,
	thaco			integer
);

create table races (
	idnum			integer primary key,
	name			varchar(40),
	pc				boolean,
	lifespan		integer,
	add_str			integer,
	add_int			integer,
	add_wis			integer,
	add_dex			integer,
	add_con			integer,
	add_cha			integer,
	max_str			integer,
	max_int			integer,
	max_wis			integer,
	max_dex			integer,
	max_con			integer,
	max_cha			integer
);

create table races_classes (
	race			integer references races(idnum),
	class			integer references classes(idnum),
	is_primary			boolean
);

create table skill_classes (
	skill			integer references skills(idnum),
	class			integer references classes(idnum),
	lvl				integer,
	gen				integer
);

create table players (
	idnum			integer primary key,
	account			integer references accounts(idnum),
	race			integer references races(idnum),
	class			integer references classes(idnum),
	remort			integer references classes(idnum),
	name			varchar(20),
	title			varchar(80),
	poofin			varchar(160),
	poofout			varchar(160),
	imm_badge		varchar(7),
	sex				char,
	hitp			integer,
	mana			integer,
	move			integer,
	maxhitp			integer,
	maxmana			integer,
	maxmove			integer,
	gold			integer,
	cash			integer,
	exp				integer,
	lvl				integer,
	height			integer,
	weight			integer,
	align			integer,
	gen				integer,
	birth_time		timestamp,
	death_time		timestamp,
	played_time		timestamp,
	login_time		timestamp,
	pkills			integer,
	mkills			integer,
	akills			integer,
	deaths			integer,
	reputation		integer,
	flag_severity	integer,
	str				integer,
	int				integer,
	wis				integer,
	dex				integer,
	con				integer,
	cha				integer,
	hunger			integer,
	thirst			integer,
	drunk			integer,
	invis_lvl		integer,
	wimpy			integer,
	lifepoints		integer,
	rent_kind		integer,
	rent_per_day	integer,
	currency		integer,
	cxn_mode		integer,
	home_town		integer,
	home_room		integer,
	load_room		integer,
	prefs_1			integer,
	prefs_2			integer,
	affects_1		integer,
	affects_2		integer,
	affects_3		integer,
	descrip			text
);

create table weapon_specs (
	player			integer references players(idnum),
	vnum			integer,
	lvl				integer
);

create table player_aliases (
	player			integer references players(idnum),
	complex			boolean,
	alias			varchar(50),
	replace			text
);

create table player_affects (
	player			integer references players(idnum),
	kind			integer,
	duration		integer,
	modifier		integer,
	location		integer,
	lvl				integer,
	instant			boolean,
	aff_bits		integer,
	aff_idx			integer
);

create table player_skills (
	player			integer references players(idnum),
	skill			integer,
	lvl				integer
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
	owner			integer references players(idnum)
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

-- INDEXES
create index accounts_name_idx on accounts (lower(name));
create index players_account on players (account);
create index players_name_idx on players (lower(name));
create index board_messages_board_idx on board_messages (board);

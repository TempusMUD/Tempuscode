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

DROP TABLE IF EXISTS Prelude_Alert;

CREATE TABLE Prelude_Alert (
 _ident BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
 messageid BIGINT UNSIGNED NOT NULL,
 analyzerid BIGINT UNSIGNED NOT NULL,
 UNIQUE INDEX (messageid, analyzerid)
) TYPE=InnoDB;



DROP TABLE IF EXISTS Prelude_AlertIdent;

CREATE TABLE Prelude_AlertIdent (
 _message_ident BIGINT UNSIGNED NOT NULL,
 _parent_type ENUM('T','C') NOT NULL, # T=ToolAlert C=CorrelationAlert
 INDEX (_parent_type, _message_ident),
 alertident BIGINT UNSIGNED NOT NULL,
 analyzerid BIGINT UNSIGNED NULL
) TYPE=InnoDB;



DROP TABLE IF EXISTS Prelude_ToolAlert;

CREATE TABLE Prelude_ToolAlert (
 _message_ident BIGINT UNSIGNED NOT NULL PRIMARY KEY,
 name VARCHAR(255) NOT NULL,
 command VARCHAR(255) NULL
) TYPE=InnoDB;



DROP TABLE IF EXISTS Prelude_CorrelationAlert;

CREATE TABLE Prelude_CorrelationAlert (
 _message_ident BIGINT UNSIGNED NOT NULL PRIMARY KEY,
 name VARCHAR(255) NOT NULL
) TYPE=InnoDB;



DROP TABLE IF EXISTS Prelude_OverflowAlert;

CREATE TABLE Prelude_OverflowAlert (
 _message_ident BIGINT UNSIGNED NOT NULL PRIMARY KEY,
 program VARCHAR(255) NOT NULL,
 size INTEGER UNSIGNED NULL,
 buffer BLOB NULL
) TYPE=InnoDB;



DROP TABLE IF EXISTS Prelude_Heartbeat;

CREATE TABLE Prelude_Heartbeat (
 _ident BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
 messageid BIGINT UNSIGNED NOT NULL,
 analyzerid BIGINT UNSIGNED NOT NULL,
 UNIQUE INDEX(messageid, analyzerid)
) TYPE=InnoDB;



DROP TABLE IF EXISTS Prelude_Analyzer;

CREATE TABLE Prelude_Analyzer (
 _message_ident BIGINT UNSIGNED NOT NULL,
 _parent_type ENUM('A','H') NOT NULL, # A=Alert H=Hearbeat
 _depth TINYINT UNSIGNED NOT NULL,
 PRIMARY KEY (_message_ident,_parent_type,_depth),
 analyzerid BIGINT UNSIGNED NOT NULL,
 name VARCHAR(255) NULL,
 manufacturer VARCHAR(255) NULL,
 model VARCHAR(255) NULL,
 version VARCHAR(255) NULL,
 class VARCHAR(255) NULL,
 ostype VARCHAR(255) NULL,
 osversion VARCHAR(255) NULL,
 INDEX (model),
 INDEX (analyzerid)
) TYPE=InnoDB;



DROP TABLE IF EXISTS Prelude_Classification;

CREATE TABLE Prelude_Classification (
 _message_ident BIGINT UNSIGNED NOT NULL PRIMARY KEY,
 ident BIGINT UNSIGNED NOT NULL,
 text VARCHAR(255) NOT NULL,
 INDEX (text)
) TYPE=InnoDB;



DROP TABLE IF EXISTS Prelude_Reference;

CREATE TABLE Prelude_Reference (
 _message_ident BIGINT UNSIGNED NOT NULL,
 INDEX(_message_ident),
 origin ENUM("unknown","vendor-specific","user-specific","bugtraqid","cve","osvdb") NOT NULL,
 name VARCHAR(255) NOT NULL,
 url VARCHAR(255) NOT NULL,
 meaning VARCHAR(255) NULL,
 INDEX(name)
) TYPE=InnoDB;



DROP TABLE IF EXISTS Prelude_Source;

CREATE TABLE Prelude_Source (
 _message_ident BIGINT UNSIGNED NOT NULL,
 _index TINYINT UNSIGNED NOT NULL,
 PRIMARY KEY (_message_ident, _index),
 ident BIGINT UNSIGNED NOT NULL,
 spoofed ENUM("unknown","yes","no") NOT NULL,
 interface VARCHAR(255) NULL
) TYPE=InnoDB;



DROP TABLE IF EXISTS Prelude_Target;

CREATE TABLE Prelude_Target (
 _message_ident BIGINT UNSIGNED NOT NULL,
 _index TINYINT UNSIGNED NOT NULL,
 PRIMARY KEY (_message_ident, _index),
 ident BIGINT UNSIGNED NOT NULL,
 decoy ENUM("unknown","yes","no") NOT NULL,
 interface VARCHAR(255) NULL
) TYPE=InnoDB;



DROP TABLE IF EXISTS Prelude_File;

CREATE TABLE Prelude_File (
 _message_ident BIGINT UNSIGNED NOT NULL,
 _target_index TINYINT UNSIGNED NOT NULL,
 _index TINYINT UNSIGNED NOT NULL,
 PRIMARY KEY (_message_ident, _target_index, _index),
 ident BIGINT UNSIGNED NOT NULL,
 path VARCHAR(255) NOT NULL,
 name VARCHAR(255) NOT NULL,
 category ENUM("current", "original") NULL,
 create_time DATETIME NULL,
 create_time_gmtoff INTEGER UNSIGNED NULL,
 modify_time DATETIME NULL,
 modify_time_gmtoff INTEGER UNSIGNED NULL,
 access_time DATETIME NULL,
 access_time_gmtoff INTEGER UNSIGNED NULL,
 data_size INT UNSIGNED NULL,
 disk_size INT UNSIGNED NULL,
 fstype ENUM("ufs", "efs", "nfs", "afs", "ntfs", "fat16", "fat32", "pcfs", "joliet", "iso9660") NULL
) TYPE=InnoDB;



DROP TABLE IF EXISTS Prelude_FileAccess;

CREATE TABLE Prelude_FileAccess (
 _message_ident BIGINT UNSIGNED NOT NULL,
 _target_index TINYINT UNSIGNED NOT NULL,
 _file_index TINYINT UNSIGNED NOT NULL,
 _index TINYINT UNSIGNED NOT NULL,
 PRIMARY KEY (_message_ident, _target_index, _file_index, _index)
) TYPE=InnoDB;



DROP TABLE IF EXISTS Prelude_FileAccess_Permission;

CREATE TABLE Prelude_FileAccess_Permission (
 _message_ident BIGINT UNSIGNED NOT NULL,
 _target_index TINYINT UNSIGNED NOT NULL,
 _file_index TINYINT UNSIGNED NOT NULL,
 _file_access_index TINYINT UNSIGNED NOT NULL,
 INDEX(_message_ident, _target_index, _file_index, _file_access_index),
 perm VARCHAR(255) NOT NULL
) TYPE=InnoDB;



DROP TABLE IF EXISTS Prelude_Linkage;

CREATE TABLE Prelude_Linkage (
 _message_ident BIGINT UNSIGNED NOT NULL,
 _target_index TINYINT UNSIGNED NOT NULL,
 _file_index TINYINT UNSIGNED NOT NULL,
 INDEX (_message_ident, _target_index, _file_index),
 category ENUM("hard-link","mount-point","reparse-point","shortcut","stream","symbolic-link") NOT NULL,
 name VARCHAR(255) NOT NULL,
 path VARCHAR(255) NOT NULL
) TYPE=InnoDB;



DROP TABLE IF EXISTS Prelude_Inode;

CREATE TABLE Prelude_Inode (
 _message_ident BIGINT UNSIGNED NOT NULL,
 _target_index BIGINT UNSIGNED NOT NULL,
 _file_index TINYINT UNSIGNED NOT NULL,
 PRIMARY KEY (_message_ident, _target_index, _file_index),
 change_time DATETIME NULL,
 change_time_gmtoff INTEGER UNSIGNED NULL, 
 number INT UNSIGNED NULL,
 major_device INT UNSIGNED NULL,
 minor_device INT UNSIGNED NULL,
 c_major_device INT UNSIGNED NULL,
 c_minor_device INT UNSIGNED NULL
) TYPE=InnoDB;



DROP TABLE IF EXISTS Prelude_Checksum;

CREATE TABLE Prelude_Checksum (
 _message_ident BIGINT UNSIGNED NOT NULL,
 _target_index BIGINT UNSIGNED NOT NULL,
 _file_index TINYINT UNSIGNED NOT NULL,
 PRIMARY KEY (_message_ident, _target_index, _file_index),
 algorithm ENUM("md4", "md5", "sha1", "sha2-256", "sha2-384", "sha2-512", "crc-32", "haval", "tiger", "gost") NOT NULL,
 value VARCHAR(255) NOT NULL,
 checksum_key VARCHAR(255) NULL # key is a reserved word
) TYPE=InnoDB;


DROP TABLE IF EXISTS Prelude_Impact;

CREATE TABLE Prelude_Impact (
 _message_ident BIGINT UNSIGNED NOT NULL PRIMARY KEY,
 description VARCHAR(255) NULL,
 severity ENUM("low","medium","high") NULL,
 completion ENUM("failed", "succeeded") NULL,
 type ENUM("admin", "dos", "file", "recon", "user", "other") NOT NULL
) TYPE=InnoDB;



DROP TABLE IF EXISTS Prelude_Action;

CREATE TABLE Prelude_Action (
 _message_ident BIGINT UNSIGNED NOT NULL,
 INDEX(_message_ident),
 description VARCHAR(255) NULL,
 category ENUM("block-installed", "notification-sent", "taken-offline", "other") NOT NULL
) TYPE=InnoDB;



DROP TABLE IF EXISTS Prelude_Confidence;

CREATE TABLE Prelude_Confidence (
 _message_ident BIGINT UNSIGNED NOT NULL PRIMARY KEY,
 confidence FLOAT NULL,
 rating ENUM("low", "medium", "high", "numeric") NOT NULL
) TYPE=InnoDB;



DROP TABLE IF EXISTS Prelude_Assessment;

CREATE TABLE Prelude_Assessment (
 _message_ident BIGINT UNSIGNED NOT NULL PRIMARY KEY
) TYPE=InnoDB;



DROP TABLE IF EXISTS Prelude_AdditionalData;

CREATE TABLE Prelude_AdditionalData (
 _message_ident BIGINT UNSIGNED NOT NULL,
 _parent_type ENUM('A', 'H') NOT NULL,
 INDEX (_parent_type, _message_ident),
 type ENUM("boolean","byte","character","date-time","integer","ntpstamp","portlist","real","string","byte-string","xml") NOT NULL,
 meaning VARCHAR(255) NULL,
 data BLOB NULL
) TYPE=InnoDB;



DROP TABLE IF EXISTS Prelude_CreateTime;

CREATE TABLE Prelude_CreateTime (
 _message_ident BIGINT UNSIGNED NOT NULL,
 _parent_type ENUM('A','H') NOT NULL, # A=Alert H=Hearbeat
 PRIMARY KEY (_parent_type,_message_ident),
 time DATETIME NOT NULL,
 usec INTEGER UNSIGNED NOT NULL,
 gmtoff INTEGER UNSIGNED NOT NULL,
 INDEX (time)
) TYPE=InnoDB;



DROP TABLE IF EXISTS Prelude_DetectTime;

CREATE TABLE Prelude_DetectTime (
 _message_ident BIGINT UNSIGNED NOT NULL PRIMARY KEY,
 time DATETIME NOT NULL,
 usec INTEGER UNSIGNED NOT NULL,
 gmtoff INTEGER UNSIGNED NOT NULL,	
 INDEX (time)
) TYPE=InnoDB;



DROP TABLE IF EXISTS Prelude_AnalyzerTime;

CREATE TABLE Prelude_AnalyzerTime (
 _message_ident BIGINT UNSIGNED NOT NULL,
 _parent_type ENUM('A','H') NOT NULL, # A=Alert H=Hearbeat
 PRIMARY KEY (_parent_type, _message_ident),
 time DATETIME NOT NULL,
 usec INTEGER UNSIGNED NOT NULL,
 gmtoff INTEGER UNSIGNED NOT NULL,
 INDEX (time)
) TYPE=InnoDB;



DROP TABLE IF EXISTS Prelude_Node;

CREATE TABLE Prelude_Node (
 _message_ident BIGINT UNSIGNED NOT NULL,
 _parent_type ENUM('A','H','S','T') NOT NULL, # A=Analyzer T=Target S=Source H=Heartbeat
 _parent_index TINYINT UNSIGNED NOT NULL,
 PRIMARY KEY(_parent_type, _message_ident, _parent_index),
 ident BIGINT UNSIGNED NOT NULL,
 category ENUM("unknown","ads","afs","coda","dfs","dns","hosts","kerberos","nds","nis","nisplus","nt","wfw") NULL,
 location VARCHAR(255) NULL,
 name VARCHAR(255) NULL,
 INDEX(name)
) TYPE=InnoDB;



DROP TABLE IF EXISTS Prelude_Address;

CREATE TABLE Prelude_Address (
 _message_ident BIGINT UNSIGNED NOT NULL,
 _parent_type ENUM('A','H','S','T') NOT NULL, # A=Analyser T=Target S=Source H=Heartbeat
 _parent_index TINYINT UNSIGNED NOT NULL,
 INDEX (_parent_type, _message_ident, _parent_index),
 ident BIGINT UNSIGNED NOT NULL,
 category ENUM("unknown","atm","e-mail","lotus-notes","mac","sna","vm","ipv4-addr","ipv4-addr-hex","ipv4-net","ipv4-net-mask","ipv6-addr","ipv6-addr-hex","ipv6-net","ipv6-net-mask") NOT NULL,
 vlan_name VARCHAR(255) NULL,
 vlan_num INTEGER UNSIGNED NULL,
 address VARCHAR(255) NOT NULL,
 netmask VARCHAR(255) NULL,
 INDEX (address)
) TYPE=InnoDB;



DROP TABLE IF EXISTS Prelude_User;

CREATE TABLE Prelude_User (
 _message_ident BIGINT UNSIGNED NOT NULL,
 _parent_type ENUM('S','T') NOT NULL, # T=Target S=Source
 _parent_index TINYINT UNSIGNED NOT NULL,
 PRIMARY KEY (_parent_type, _message_ident, _parent_index),
 ident BIGINT UNSIGNED NOT NULL,
 category ENUM("unknown","application","os-device") NOT NULL
) TYPE=InnoDB;



DROP TABLE IF EXISTS Prelude_UserId;

CREATE TABLE Prelude_UserId (
 _message_ident BIGINT UNSIGNED NOT NULL,
 _parent_type ENUM('S','T', 'F') NOT NULL, # T=Target User S=Source User F=File Access 
 _parent_index TINYINT UNSIGNED NOT NULL,
 _file_index TINYINT UNSIGNED NOT NULL,
 _file_access_index TINYINT UNSIGNED NOT NULL,
 INDEX (_parent_type, _message_ident, _parent_index, _file_index, _file_access_index), # _file_index and _file_access_index will always be zero if parent_type = 'F'
 ident BIGINT UNSIGNED NOT NULL,
 type ENUM("current-user","original-user","target-user","user-privs","current-group","group-privs","other-privs") NOT NULL,
 name VARCHAR(255) NULL,
 number INTEGER UNSIGNED NULL
) TYPE=InnoDB;



DROP TABLE IF EXISTS Prelude_Process;

CREATE TABLE Prelude_Process (
 _message_ident BIGINT UNSIGNED NOT NULL,
 _parent_type ENUM('A','H','S','T') NOT NULL, # A=Analyzer T=Target S=Source H=Heartbeat
 _parent_index TINYINT UNSIGNED NOT NULL,
 PRIMARY KEY (_parent_type, _message_ident, _parent_index),
 ident BIGINT UNSIGNED NOT NULL,
 name VARCHAR(255) NOT NULL,
 pid INTEGER UNSIGNED NULL,
 path VARCHAR(255) NULL
) TYPE=InnoDB;



DROP TABLE IF EXISTS Prelude_ProcessArg;

CREATE TABLE Prelude_ProcessArg (
 _message_ident BIGINT UNSIGNED NOT NULL,
 _parent_type ENUM('A','H','S','T') NOT NULL DEFAULT 'A', # A=Analyser T=Target S=Source
 _parent_index TINYINT UNSIGNED NOT NULL,
 INDEX (_parent_type, _message_ident, _parent_index),
 arg VARCHAR(255) NOT NULL
) TYPE=InnoDB;



DROP TABLE IF EXISTS Prelude_ProcessEnv;

CREATE TABLE Prelude_ProcessEnv (
 _message_ident BIGINT UNSIGNED NOT NULL,
 _parent_type ENUM('A','H','S','T') NOT NULL, # A=Analyser T=Target S=Source
 _parent_index BIGINT UNSIGNED NOT NULL,
 INDEX (_parent_type, _message_ident, _parent_index),
 env VARCHAR(255) NOT NULL
) TYPE=InnoDB;



DROP TABLE IF EXISTS Prelude_Service;

CREATE TABLE Prelude_Service (
 _message_ident BIGINT UNSIGNED NOT NULL,
 _parent_type ENUM('S','T') NOT NULL, # T=Target S=Source
 _parent_index BIGINT UNSIGNED NOT NULL,
 PRIMARY KEY (_parent_type, _message_ident, _parent_index),
 ident BIGINT UNSIGNED NOT NULL,
 ip_version TINYINT UNSIGNED NULL,
 name VARCHAR(255) NULL,
 port SMALLINT UNSIGNED NULL,
 iana_protocol_number TINYINT UNSIGNED NULL,
 iana_protocol_name VARCHAR(255) NULL,
 portlist VARCHAR (255) NULL,
 protocol VARCHAR(255) NULL,
 INDEX (protocol(10),port),
 INDEX (protocol(10),name(10))
) TYPE=InnoDB;



DROP TABLE IF EXISTS Prelude_WebService;

CREATE TABLE Prelude_WebService (
 _message_ident BIGINT UNSIGNED NOT NULL,
 _parent_type ENUM('S','T') NOT NULL, # T=Target S=Source
 _parent_index TINYINT UNSIGNED NOT NULL,
 PRIMARY KEY (_parent_type, _message_ident, _parent_index),
 url VARCHAR(255) NOT NULL,
 cgi VARCHAR(255) NULL,
 http_method VARCHAR(255) NULL
) TYPE=InnoDB;



DROP TABLE IF EXISTS Prelude_WebServiceArg;

CREATE TABLE Prelude_WebServiceArg (
 _message_ident BIGINT UNSIGNED NOT NULL,
 _parent_type ENUM('S','T') NOT NULL, # T=Target S=Source
 _parent_index TINYINT UNSIGNED NOT NULL,
 PRIMARY KEY (_parent_type, _message_ident, _parent_index),
 arg VARCHAR(255) NOT NULL
) TYPE=InnoDB;



DROP TABLE IF EXISTS Prelude_SNMPService;

CREATE TABLE Prelude_SNMPService (
 _message_ident BIGINT UNSIGNED NOT NULL,
 _parent_type ENUM('S','T') NOT NULL, # T=Target S=Source
 _parent_index TINYINT UNSIGNED NOT NULL,
 PRIMARY KEY (_parent_type, _message_ident, _parent_index),
 oid VARCHAR(255) NULL,
 community VARCHAR(255) NULL,
 security_name VARCHAR(255) NULL,
 context_name VARCHAR(255) NULL,
 context_engine_id VARCHAR(255) NULL,
 command VARCHAR(255) NULL
) TYPE=InnoDB;

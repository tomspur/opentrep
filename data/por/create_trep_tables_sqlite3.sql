--
-- ORI-maintained list of POR (points of reference, i.e., airports, cities,
-- places, etc.)
-- See http://github.com/opentraveldata/optd/tree/trunk/refdata/ORI
--

drop table if exists ori_por;
create table ori_por (
 pk varchar(20) NOT NULL,
 location_type varchar(4) default NULL,
 iata_code varchar(3) default NULL,
 icao_code varchar(4) default NULL,
 faa_code varchar(4) default NULL,
 is_geonames varchar(1) default NULL,
 geoname_id int(11) default NULL,
 envelope_id int(11) default NULL,
 date_from date default NULL,
 date_until date default NULL,
 serialised_place varchar(8000) default NULL,
 primary key (pk)
);

--
-- SQLite3 standard load statement (however, there is no correspondance
-- between the table and CSV file formats)
--
-- delete from ori_por;
-- .separator '^'
-- .import ori_por_public.csv ori_por


--
-- Indexes
--
create index ori_por_iata_code on ori_por (iata_code);
create index ori_por_iata_date on ori_por (iata_code, date_from, date_until);
create index ori_por_icao_code on ori_por (icao_code);
create index ori_por_geonameid on ori_por (geoname_id);

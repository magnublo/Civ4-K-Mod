#!/usr/bin/env python3
"""Port the unit and building art of Realism Invictus (RI) to K-Mod-Magnus.

Reads an unpacked Realism Invictus mod (its "Unpacked" installer extracted with 7-Zip: the folder that holds
Assets), the Civilization IV installation (to know which art files the base game already has) and this
repository, and writes into the repository:

  Assets/xml/Art/RI_CIV4ArtDefines_Unit.xml       RI's unit art defines, types prefixed RI_
  Assets/xml/Art/RI_CIV4ArtDefines_Building.xml   RI's building art defines, types prefixed RI_
  Assets/xml/Art/RI_CIV4ArtInfos.xml              which RI art each K-Mod unit and building gets, per civilization,
                                                  and each civilization's RI city art style (+ its schema,
                                                  RI_CIV4ArtSchema.xml)
  Assets/xml/Buildings/CIV4CityLSystem.xml        BtS's city layout plus RI's (building placements, houses, walls)
  Assets/xml/GlobalTypes.xml                      BtS's, plus RI's city art styles
  Assets/Art/RI/...                               the RI files these need (RI's Art/X goes to Art/RI/X)
  Assets/Art/Structures/Cities/RI/...             RI's city houses (the game looks for them in Art/Structures/Cities)

The DLL switches between K-Mod's art and this one (option "Realism Invictus graphics"). With the option off the
game looks as before: RI files live only in the two RI folders, RI's types, city layouts and art styles are
prefixed RI_, and BtS's houses are limited to BtS's art styles.

Usage: port_ri_art.py --ri RI_FOLDER --civ4 CIV4_FOLDER [--mod REPO] [--dry-run] [--report FILE]
  RI_FOLDER    folder that holds RI's Assets (e.g. "$_4_" in the extracted installer)
  CIV4_FOLDER  Civilization IV folder that holds Assets, Warlords and Beyond the Sword
"""

import argparse
import collections
import copy
import hashlib
import os
import re
import shutil
import struct
import sys
import xml.etree.ElementTree as ET

TIERS = ('Early', 'Classical', 'Middle', 'Renn', 'Industrial', 'Late', 'Future')
PREFIX = 'RI_'
DEST = 'Art/RI/'  # RI's Art/X goes to Art/RI/X
CITIES = 'art/structures/cities/'  # GENERIC_BUILDING_DIR: where the game looks for city houses and lots
CITIES_SUB = 'RI/'  # RI's go to Art/Structures/Cities/RI/

# K-Mod civilization -> RI civilization whose look it gets
CIV_MAP = {
	'CIVILIZATION_AMERICA': 'CIVILIZATION_AMERICA',
	'CIVILIZATION_ARABIA': 'CIVILIZATION_ARABIA',
	'CIVILIZATION_AZTEC': 'CIVILIZATION_AZTEC',
	'CIVILIZATION_BABYLON': 'CIVILIZATION_BABYLON',
	'CIVILIZATION_BYZANTIUM': 'CIVILIZATION_GREECE',  # RI's Greece turns Byzantine (Kataphraktoi, Varangian Guard)
	'CIVILIZATION_CARTHAGE': 'CIVILIZATION_CARTHAGE',
	'CIVILIZATION_CELT': 'CIVILIZATION_CELT',
	'CIVILIZATION_CHINA': 'CIVILIZATION_CHINA',
	'CIVILIZATION_EGYPT': 'CIVILIZATION_EGYPT',
	'CIVILIZATION_ENGLAND': 'CIVILIZATION_ENGLAND',
	'CIVILIZATION_ETHIOPIA': 'CIVILIZATION_ETHIOPIA',
	'CIVILIZATION_FRANCE': 'CIVILIZATION_FRANCE',
	'CIVILIZATION_GERMANY': 'CIVILIZATION_GERMANY',
	'CIVILIZATION_GREECE': 'CIVILIZATION_GREECE',
	'CIVILIZATION_HOLY_ROMAN': 'CIVILIZATION_AUSTRIA',  # Habsburg: "Arquebusier (HRE)", K.u.K. line infantry
	'CIVILIZATION_INCA': 'CIVILIZATION_INCA',
	'CIVILIZATION_INDIA': 'CIVILIZATION_INDIA',
	'CIVILIZATION_JAPAN': 'CIVILIZATION_JAPAN',
	'CIVILIZATION_KHMER': 'CIVILIZATION_KHMER',
	'CIVILIZATION_KOREA': 'CIVILIZATION_KOREA',
	'CIVILIZATION_MALI': 'CIVILIZATION_MALI',
	'CIVILIZATION_MAYA': 'CIVILIZATION_MAYA',
	'CIVILIZATION_MONGOL': 'CIVILIZATION_MONGOL',
	'CIVILIZATION_NATIVE_AMERICA': 'CIVILIZATION_IROQUOIS',
	'CIVILIZATION_NETHERLANDS': 'CIVILIZATION_NETHERLANDS',
	'CIVILIZATION_OTTOMAN': 'CIVILIZATION_OTTOMAN',
	'CIVILIZATION_PERSIA': 'CIVILIZATION_PERSIA',
	'CIVILIZATION_PORTUGAL': 'CIVILIZATION_PORTUGAL',
	'CIVILIZATION_ROME': 'CIVILIZATION_ROME',
	'CIVILIZATION_RUSSIA': 'CIVILIZATION_RUSSIA',
	'CIVILIZATION_SPAIN': 'CIVILIZATION_SPAIN',
	'CIVILIZATION_SUMERIA': 'CIVILIZATION_BABYLON',  # RI's "Mesopotamian Empire"
	'CIVILIZATION_VIKING': 'CIVILIZATION_VIKING',
	'CIVILIZATION_ZULU': 'CIVILIZATION_ZULU',
	'CIVILIZATION_MINOR': 'CIVILIZATION_MINOR',
	'CIVILIZATION_BARBARIAN': 'CIVILIZATION_BARBARIAN',
}

# K-Mod unit class -> RI unit class where the names differ. None: no RI class looks like it; the unit gets RI's
# unit of the same name if there is one, else keeps K-Mod's art.
CLASS_MAP = {
	'UNITCLASS_TANK': 'UNITCLASS_EARLY_MEDIUM_TANK',  # RI "Medium Tank" (Pz.III, M11/39, ...)
	'UNITCLASS_SHIP_OF_THE_LINE': 'UNITCLASS_LINE',
	'UNITCLASS_STEALTH_DESTROYER': 'UNITCLASS_ECM_DESTROYER',  # RI "Modern Destroyer"
	'UNITCLASS_MISSILE_CRUISER': 'UNITCLASS_AEGIS',  # RI "Missile Cruiser"
	'UNITCLASS_AIRSHIP': 'UNITCLASS_ZEPPELIN_WORLD',  # RI "Zeppelin"
	'UNITCLASS_GALLEON': None,  # RI's UNIT_GALLEON is a "Merchantman"; used as is
	'UNITCLASS_SAM_INFANTRY': None,  # RI's class is vehicles
	'UNITCLASS_CONFUCIAN_MISSIONARY': None,  # no Confucianism in RI
	'UNITCLASS_EXECUTIVE_1': None, 'UNITCLASS_EXECUTIVE_2': None, 'UNITCLASS_EXECUTIVE_3': None,
	'UNITCLASS_EXECUTIVE_4': None, 'UNITCLASS_EXECUTIVE_5': None, 'UNITCLASS_EXECUTIVE_6': None,
	'UNITCLASS_EXECUTIVE_7': None,  # no corporations in RI
}

# K-Mod building class -> RI building class where the names differ (None: keep K-Mod's art)
BUILDING_CLASS_MAP = {}

# XDR schema of RI_CIV4ArtInfos.xml (the game validates its XML against the schema the root element names)
SCHEMA = """<?xml version="1.0" encoding="UTF-8"?>
<!-- Generated by tools/ri_art/port_ri_art.py. Do not edit by hand. -->
<!-- Schema of RI_CIV4ArtInfos.xml: Realism Invictus art per unit, building and civilization (K-Mod-Magnus) -->
<Schema xmlns="urn:schemas-microsoft-com:xml-data" xmlns:dt="urn:schemas-microsoft-com:datatypes">
	<ElementType name="UnitType" content="textOnly"/>
	<ElementType name="BuildingType" content="textOnly"/>
	<ElementType name="CivilizationType" content="textOnly"/>
	<ElementType name="ArtStyleType" content="textOnly"/>
	<ElementType name="ArtDefineTag" content="textOnly"/>
%(tiers)s	<ElementType name="fMaxSpeed" content="textOnly" dt:type="float"/>
	<ElementType name="fPadTime" content="textOnly" dt:type="float"/>
	<ElementType name="iRequired" content="textOnly" dt:type="int"/>
	<ElementType name="Civilizations" content="eltOnly">
		<element type="CivilizationType" minOccurs="0" maxOccurs="*"/>
	</ElementType>
	<ElementType name="UnitMeshGroup" content="eltOnly">
		<element type="iRequired"/>
%(tier_refs)s	</ElementType>
	<ElementType name="UnitMeshGroups" content="eltOnly">
		<element type="UnitMeshGroup" maxOccurs="*"/>
	</ElementType>
	<ElementType name="RIUnitArtInfo" content="eltOnly">
		<element type="UnitType"/>
		<element type="Civilizations"/>
		<element type="fMaxSpeed"/>
		<element type="fPadTime"/>
		<element type="UnitMeshGroups"/>
	</ElementType>
	<ElementType name="RIUnitArtInfos" content="eltOnly">
		<element type="RIUnitArtInfo" minOccurs="0" maxOccurs="*"/>
	</ElementType>
	<ElementType name="RIBuildingArtInfo" content="eltOnly">
		<element type="BuildingType"/>
		<element type="Civilizations"/>
		<element type="ArtDefineTag"/>
	</ElementType>
	<ElementType name="RIBuildingArtInfos" content="eltOnly">
		<element type="RIBuildingArtInfo" minOccurs="0" maxOccurs="*"/>
	</ElementType>
	<ElementType name="RICivilizationArtInfo" content="eltOnly">
		<element type="CivilizationType"/>
		<element type="ArtStyleType"/>
	</ElementType>
	<ElementType name="RICivilizationArtInfos" content="eltOnly">
		<element type="RICivilizationArtInfo" minOccurs="0" maxOccurs="*"/>
	</ElementType>
	<ElementType name="Civ4RIArtInfos" content="eltOnly">
		<element type="RIUnitArtInfos"/>
		<element type="RIBuildingArtInfos"/>
		<element type="RICivilizationArtInfos"/>
	</ElementType>
</Schema>
""" % dict(
	tiers=''.join('\t<ElementType name="%sArtDefineTag" content="textOnly"/>\n' % t for t in TIERS),
	tier_refs=''.join('\t\t<element type="%sArtDefineTag" minOccurs="0"/>\n' % t for t in TIERS))

IMAGE_RE = rb'\.(?:dds|tga|bmp|png|jpg)'


def warn(msg):
	sys.stderr.write('warning: %s\n' % msg)


# ---------------------------------------------------------------------------------------------------------------
# XML

def parse_xml(path):
	data = open(path, 'rb').read()
	data = re.sub(rb'\sxmlns="[^"]*"', b'', data, count=1)
	try:
		return ET.fromstring(data)
	except ET.ParseError:
		txt = re.sub(r'^<\?xml[^>]*\?>', '', data.decode('latin-1'))
		return ET.fromstring(txt)


def text(el, tag, default=None):
	c = el.find(tag)
	if c is None or c.text is None:
		return default
	return c.text.strip()


def find_files(base, pattern):
	out = []
	for dirpath, dirs, files in os.walk(base):
		for f in files:
			if re.fullmatch(pattern, f, re.I):
				out.append(os.path.join(dirpath, f))
	return sorted(out)


HEADER = '<!-- Generated by tools/ri_art/port_ri_art.py from Realism Invictus %s. Do not edit by hand. -->\n'


def write_xml(path, root, schema, version):
	"""XML declaration, tabs, schema on the root element, like Firaxis' files"""
	root.set('xmlns', 'x-schema:' + schema)
	ET.indent(root, space='\t')
	body = ET.tostring(root, encoding='unicode')
	write_text(path, '<?xml version="1.0" encoding="UTF-8"?>\n' + HEADER % version + body + '\n')


def write_text(path, s):
	os.makedirs(os.path.dirname(path), exist_ok=True)
	with open(path, 'w', encoding='utf-8', newline='\n') as f:
		f.write(s)


def sub(parent, tag, value=None):
	e = ET.SubElement(parent, tag)
	if value is not None:
		e.text = value
	return e


# ---------------------------------------------------------------------------------------------------------------
# Art files

def fpk_entries(path):
	"""Civ4 FPK (PakBuild) archive: [(name, size, offset)]"""
	with open(path, 'rb') as f:
		ver, magic = struct.unpack('<I4s', f.read(8))
		if magic != b'FPK_':
			raise ValueError('%s is not an FPK archive' % path)
		f.read(1)
		(n,) = struct.unpack('<I', f.read(4))
		out = []
		for i in range(n):
			(ln,) = struct.unpack('<I', f.read(4))
			name = bytes((b - 1) & 0xff for b in f.read(ln)).decode('latin-1')
			f.read((4 - ln % 4) % 4)
			ft, size, off = struct.unpack('<QII', f.read(16))
			out.append((name, size, off))
	return out


def norm(p):
	return p.replace('\\', '/').lower().lstrip('/')


class FileSet:
	"""Art files by normalized path (lower case, '/'): where to read them, their real spelling"""

	def __init__(self):
		self.files = {}

	def add_dir(self, assets):
		for dirpath, dirs, files in os.walk(os.path.join(assets, 'Art')):
			for x in files:
				full = os.path.join(dirpath, x)
				rel = os.path.relpath(full, assets).replace(os.sep, '/')
				self.files[norm(rel)] = (('file', full), rel)

	def add_fpk(self, path):
		for name, size, off in fpk_entries(path):
			self.files[norm(name)] = (('fpk', path, off, size), name.replace('\\', '/'))

	def __contains__(self, p):
		return norm(p) in self.files

	def real(self, p):
		return self.files[norm(p)][1]

	def read(self, p):
		r = self.files[norm(p)][0]
		if r[0] == 'file':
			return open(r[1], 'rb').read()
		with open(r[1], 'rb') as f:
			f.seek(r[2])
			return f.read(r[3])

	def md5(self, p):
		return hashlib.md5(self.read(p)).hexdigest()

	def match(self, pattern):
		"""normalized paths matching a regex over normalized paths"""
		r = re.compile(pattern)
		return [p for p in self.files if r.fullmatch(p)]


def length_prefixed(data, ext_re):
	"""strings in a NIF/KFM ending with one of the extensions (stored as uint32 length + bytes)"""
	out = []
	for m in re.finditer(ext_re, data, re.I):
		e = m.end()
		for s in range(e - 1, max(3, e - 300), -1):
			if struct.unpack('<I', data[s - 4:s])[0] == e - s:
				out.append(data[s:e].decode('latin-1'))
				break
	return out


def basename(p):
	return re.split(r'[\\/]', p)[-1]


# ---------------------------------------------------------------------------------------------------------------
# Game data

def xml_finder(*assets_dirs):
	"""finds a mod's XML files: the first folder that has a file wins (mod before base game), as in the game"""
	def find(sub, pattern):
		found = {}
		for a in assets_dirs:
			for x in ('XML', 'xml'):
				for s in (sub, sub.lower()):
					d = os.path.join(a, x, s)
					if os.path.isdir(d):
						for p in find_files(d, pattern):
							found.setdefault(os.path.basename(p).lower(), p)
		return sorted(found.values())
	return find


class Mod:
	"""units, buildings, civilizations and art defines of a mod"""

	def __init__(self, find):
		self.find = find
		self.units = {}
		for path in find('Units', r'.*CIV4UnitInfos\.xml'):
			for u in parse_xml(path).iter('UnitInfo'):
				self.units[text(u, 'Type')] = dict(cls=text(u, 'Class'), mesh=self._mesh(u), desc=text(u, 'Description'))
		self.unit_classes = {}
		for path in find('Units', r'CIV4UnitClassInfos\.xml'):
			for c in parse_xml(path).iter('UnitClassInfo'):
				self.unit_classes[text(c, 'Type')] = text(c, 'DefaultUnit')
		self.buildings = {}
		for path in find('Buildings', r'.*CIV4BuildingInfos\.xml'):
			for b in parse_xml(path).iter('BuildingInfo'):
				self.buildings[text(b, 'Type')] = dict(cls=text(b, 'BuildingClass'), art=text(b, 'ArtDefineTag'),
					desc=text(b, 'Description'))
		self.building_classes = {}
		self.wonder_classes = set()
		for path in find('Buildings', r'CIV4BuildingClassInfos\.xml'):
			for c in parse_xml(path).iter('BuildingClassInfo'):
				self.building_classes[text(c, 'Type')] = text(c, 'DefaultBuilding')
				if int(text(c, 'iMaxGlobalInstances', '-1')) > 0 or int(text(c, 'iMaxPlayerInstances', '-1')) > 0:
					self.wonder_classes.add(text(c, 'Type'))
		self.civs = {}
		for path in find('Civilizations', r'.*CIV4CivilizationInfos\.xml'):
			for c in parse_xml(path).iter('CivilizationInfo'):
				self.civs[text(c, 'Type')] = dict(
					art_style=text(c, 'ArtStyleType'), unit_art_style=text(c, 'UnitArtStyleType'),
					units=dict((text(x, 'UnitClassType'), text(x, 'UnitType')) for x in c.findall('Units/Unit')),
					buildings=dict((text(x, 'BuildingClassType'), text(x, 'BuildingType'))
						for x in c.findall('Buildings/Building')))
		self.styles = {}
		for path in find('Civilizations', r'CIV4UnitArtStyleTypeInfos\.xml'):
			for st in parse_xml(path).iter('UnitArtStyleTypeInfo'):
				self.styles[text(st, 'Type')] = dict(
					(text(su, 'UnitType'), [self._tiers(g) for g in su.findall('UnitMeshGroup')])
					for su in st.findall('StyleUnits/StyleUnit'))
		self.unit_art = {}
		for path in find('Art', r'CIV4ArtDefines_Unit\.xml'):
			for a in parse_xml(path).iter('UnitArtInfo'):
				self.unit_art[text(a, 'Type')] = a
		self.building_art = {}
		for path in find('Art', r'CIV4ArtDefines_Building\.xml'):
			for a in parse_xml(path).iter('BuildingArtInfo'):
				self.building_art[text(a, 'Type')] = a
		self.lsystem_path = find('Buildings', r'CIV4CityLSystem\.xml')[0]
		self.global_types_path = find('', r'GlobalTypes\.xml')[0] if find('', r'GlobalTypes\.xml') else None

	@staticmethod
	def _tiers(g):
		return dict((t, text(g, t + 'ArtDefineTag')) for t in TIERS if text(g, t + 'ArtDefineTag'))

	def _mesh(self, u):
		mg = u.find('UnitMeshGroups')
		if mg is None:
			return None
		return dict(size=int(text(mg, 'iGroupSize', '0')), speed=text(mg, 'fMaxSpeed', '0'),
			pad=text(mg, 'fPadTime', '0'),
			groups=[dict(req=int(text(g, 'iRequired', '0')), tiers=self._tiers(g)) for g in mg.findall('UnitMeshGroup')])

	def civ_unit(self, civ, cls):
		"""the unit civ builds for cls. A civ that can't build cls (NONE) gets its own unit named after the class'
		default unit if it has one (RI's Rome: no Swordsman, but UNIT_SWORDSMAN_ROME "Legion (Imperial)")"""
		default = self.unit_classes.get(cls)
		c = self.civs.get(civ)
		if c is None or cls not in c['units']:
			return default
		v = c['units'][cls]
		if v and v != 'NONE':
			return v
		own = sorted(u for u in c['units'].values() if u and u != 'NONE' and default and u.startswith(default + '_'))
		return own[0] if own else default

	def civ_building(self, civ, cls):
		default = self.building_classes.get(cls)
		c = self.civs.get(civ)
		if c is None or cls not in c['buildings']:
			return default
		v = c['buildings'][cls]
		return v if v and v != 'NONE' else default


# ---------------------------------------------------------------------------------------------------------------
# The port

class Port:
	def __init__(self, args):
		self.args = args
		self.mod = os.path.abspath(args.mod)
		self.ri_dir = os.path.abspath(args.ri)
		civ4 = args.civ4
		self.bts_assets = os.path.join(civ4, 'Beyond the Sword', 'Assets')
		bases = [self.bts_assets, os.path.join(civ4, 'Warlords', 'Assets'), os.path.join(civ4, 'Assets')]
		print('reading game data ...')
		self.km = Mod(xml_finder(os.path.join(self.mod, 'Assets'), *bases))
		self.bts_lsystem = xml_finder(*bases)('Buildings', r'CIV4CityLSystem\.xml')[0]
		self.bts_global_types = [p for p in xml_finder(*bases)('', r'GlobalTypes\.xml')][0]
		self.ri = Mod(xml_finder(os.path.join(self.ri_dir, 'Assets')))
		self.version = self._ri_version()
		print('indexing art files ...')
		self.base = FileSet()  # what the game finds without RI: vanilla, Warlords, BtS, K-Mod-Magnus
		for a in reversed(bases):
			if os.path.isdir(a):
				for fpk in sorted(f for f in os.listdir(a) if re.fullmatch(r'assets\d*\.fpk', f, re.I)):
					self.base.add_fpk(os.path.join(a, fpk))
				self.base.add_dir(a)
		self.base.add_dir(os.path.join(self.mod, 'Assets'))
		self.rif = FileSet()
		self.rif.add_dir(os.path.join(self.ri_dir, 'Assets'))
		self.dir_case = {}  # normalized folder -> spelling (RI's first, then the unpacked base game's)
		for fs in (self.rif, self.base):
			for lp, (r, real) in fs.files.items():
				d, rd = os.path.dirname(lp), os.path.dirname(real)
				while d and d not in self.dir_case and rd.lower() == d and rd != d:
					self.dir_case[d] = rd
					d, rd = os.path.dirname(d), os.path.dirname(rd)
		self.audio = self._audio_tags()
		self.copies = {}  # normalized dest -> (dest, FileSet, source)
		self.dest_case = {}
		self.missing = collections.Counter()
		self.unknown_sounds = collections.Counter()
		self.reloc_cache = {}
		self.deps_cache = {}

	def _ri_version(self):
		for f in os.listdir(self.ri_dir):
			if f.lower().endswith('.ini'):
				m = re.search(r'Version\s*=\s*(\S+)', open(os.path.join(self.ri_dir, f), encoding='latin-1').read())
				if m:
					return m.group(1)
		return self.args.ri_version or 'unknown version'

	def _audio_tags(self):
		"""sound scripts, and the footstep sounds of GlobalTypes.xml (run sounds)"""
		tags = set()
		for path in xml_finder(os.path.join(self.mod, 'Assets'), self.bts_assets)('Audio', r'Audio[23]DScripts\.xml'):
			for e in parse_xml(path).iter('ScriptID'):
				if e.text:
					tags.add(e.text.strip())
		for e in parse_xml(self.bts_global_types).iter('FootstepAudioType'):
			if e.text:
				tags.add(e.text.strip())
		return tags

	# ----- units -----

	def unit_source(self, unit, civ):
		"""(RI unit, RI unit art style) whose art K-Mod's unit gets when civ owns it, or None"""
		km, ri = self.km, self.ri
		cls = km.units[unit]['cls']
		ri_cls = CLASS_MAP.get(cls, cls)
		ri_civ = CIV_MAP.get(civ)
		style = ri.civs[ri_civ]['unit_art_style'] if ri_civ in ri.civs else None
		unique = km.unit_classes.get(cls) != unit
		if unit in ri.units and (unique or ri_civ is None or ri_cls not in ri.unit_classes):
			return unit, style  # RI has this very unit
		if ri_cls not in ri.unit_classes:
			return None
		if ri_civ is None:
			return ri.unit_classes[ri_cls], None
		v = ri.civ_unit(ri_civ, ri_cls)
		return (v, style) if v in ri.units else None

	def unit_owners(self, unit):
		km = self.km
		cls = km.units[unit]['cls']
		return [c for c in km.civs if km.civs[c]['units'].get(cls, km.unit_classes.get(cls)) == unit]

	def styled_groups(self, unit, style):
		ri = self.ri
		groups = [dict(req=g['req'], tiers=dict(g['tiers'])) for g in ri.units[unit]['mesh']['groups']]
		for j, sg in enumerate(ri.styles.get(style, {}).get(unit, [])[:len(groups)]):
			groups[j]['tiers'].update(sg)  # as in RI: a style's tags replace the unit's, era by era
		return groups

	@staticmethod
	def normalize(reqs, n):
		"""how many of each figure: n in all (K-Mod's group size, so that battles play out the same with and
		without RI art), each figure group at least once if there is room, the rest shared out as in RI"""
		if n <= 0:
			return []
		if len(reqs) >= n:
			return [1] * n
		w = [max(r, 1) for r in reqs]
		q = [x * n / float(sum(w)) for x in w]
		a = [max(1, int(x)) for x in q]
		while sum(a) < n:
			a[max(range(len(a)), key=lambda k: (q[k] - a[k], -k))] += 1
		while sum(a) > n:
			a[min((k for k in range(len(a)) if a[k] > 1), key=lambda k: (q[k] - a[k], k))] -= 1
		return a

	def unit_spec(self, unit, src):
		ri_unit, style = src
		mesh = self.ri.units[ri_unit]['mesh']
		km_mesh = self.km.units[unit]['mesh']
		if not mesh or not km_mesh:
			return None
		groups = self.styled_groups(ri_unit, style)
		for g in groups:
			for t in list(g['tiers']):
				if g['tiers'][t] not in self.ri.unit_art:
					warn('%s: RI art %s is not defined' % (ri_unit, g['tiers'][t]))
					del g['tiers'][t]
		groups = [g for g in groups if g['tiers']]
		if not groups:
			return None
		reqs = self.normalize([g['req'] for g in groups], km_mesh['size'])
		return dict(src=ri_unit, style=style, speed=mesh['speed'], pad=mesh['pad'],
			groups=[(r, tuple((t, g['tiers'][t]) for t in TIERS if t in g['tiers'])) for r, g in zip(reqs, groups)])

	@staticmethod
	def spec_key(s):
		return (s['speed'], s['pad'], tuple(s['groups']))

	def port_units(self):
		km = self.km
		self.unit_specs = []  # (unit, [civs]; [] = default, spec)
		self.unit_report = []
		for unit in km.units:
			owners = self.unit_owners(unit)
			unique = km.unit_classes.get(km.units[unit]['cls']) != unit
			# the default (for civs without an entry of their own); a unique unit's is its owner's look
			src = self.unit_source(unit, owners[0] if unique and owners else None)
			default = self.unit_spec(unit, src) if src else None
			per = collections.OrderedDict()
			for civ in owners:
				s = self.unit_source(unit, civ) if civ in CIV_MAP else None
				spec = self.unit_spec(unit, s) if s else None
				if spec is None or (default is not None and self.spec_key(spec) == self.spec_key(default)):
					continue
				per.setdefault(self.spec_key(spec), (spec, []))[1].append(civ)
			if default is not None:
				self.unit_specs.append((unit, [], default))
			for spec, civs in per.values():
				self.unit_specs.append((unit, civs, spec))
			self.unit_report.append((unit, default, [(civs, spec) for spec, civs in per.values()]))

	# ----- buildings -----

	def building_source(self, building, civ):
		"""RI building whose art K-Mod's building gets in civ's cities, or None. Unique buildings get RI's of the
		same name or keep their own; others RI's version for the civ. For wonders only a version of the same wonder
		(BUILDING_HEROIC_EPIC_CHINA for the Heroic Epic), not another landmark (Arc de Triomphe for Mt. Rushmore)."""
		km, ri = self.km, self.ri
		cls = km.buildings[building]['cls']
		ri_cls = BUILDING_CLASS_MAP.get(cls, cls)
		unique = km.building_classes.get(cls) != building
		if unique or ri_cls not in ri.building_classes:
			return building if building in ri.buildings else None
		default = building if building in ri.buildings else ri.building_classes.get(ri_cls)
		ri_civ = CIV_MAP.get(civ)
		if ri_civ is None:
			return default if default in ri.buildings else None
		v = ri.civ_building(ri_civ, ri_cls)
		if v == ri.building_classes.get(ri_cls):
			v = default
		elif ri_cls in ri.wonder_classes and not v.startswith((default or '') + '_'):
			v = default
		return v if v in ri.buildings else None

	def port_buildings(self):
		km, ri = self.km, self.ri
		self.building_specs = []  # (building, [civs]; [] = default, RI art define)
		self.building_report = []
		for b in km.buildings:
			cls = km.buildings[b]['cls']
			owners = [c for c in km.civs if km.civs[c]['buildings'].get(cls, km.building_classes.get(cls)) == b]
			unique = km.building_classes.get(cls) != b
			src = self.building_source(b, owners[0] if unique and owners else None)
			default = ri.buildings[src]['art'] if src else None
			if default not in ri.building_art:
				default = None
			if default:
				self.building_specs.append((b, [], default))
			per = collections.OrderedDict()
			for civ in owners:
				s = self.building_source(b, civ) if civ in CIV_MAP else None
				art = ri.buildings[s]['art'] if s else None
				if art and art in ri.building_art and art != default:
					per.setdefault(art, []).append(civ)
			for art, civs in per.items():
				self.building_specs.append((b, civs, art))
			self.building_report.append((b, src, default, list(per.items())))

	# ----- art files -----

	def ri_modified(self, p):
		"""RI has its own version of this file"""
		return p in self.rif and (p not in self.base or self.rif.md5(p) != self.base.md5(p))

	def source(self, p):
		"""the version RI uses: (FileSet, path) or None"""
		if p in self.rif:
			return self.rif, p
		if p in self.base:
			return self.base, p
		return None

	def deps(self, p):
		"""files a model loads from its own folder by bare name: textures of a NIF; animations of a KFM and the model
		it names, with that model's textures (the unit picture in the interface loads the KFM's model). Shared
		textures (Art/Shared) that RI changed are listed as ('shared', path, name)."""
		lp = norm(p)
		if lp in self.deps_cache:
			return self.deps_cache[lp]
		out = []
		self.deps_cache[lp] = out
		src = self.source(lp)
		if src is not None:
			kfm = lp.endswith('.kfm')
			folder = os.path.dirname(lp)
			for n in length_prefixed(src[0].read(lp), rb'\.(?:kf|nif)\b' if kfm else IMAGE_RE):
				b = basename(n).lower()
				if kfm and not b.endswith(('.kf', '.nif')):
					continue
				cand = folder + '/' + b
				if cand in self.rif or cand in self.base:
					if cand not in out:
						out.append(cand)
					if cand.endswith('.nif'):
						out.extend(d for d in self.deps(cand) if d not in out)
				elif not kfm and self.ri_modified('art/shared/' + b):
					out.append(('shared', 'art/shared/' + b, basename(self.rif.real('art/shared/' + b))))
		return out

	def needs_reloc(self, p):
		"""RI's version of this file (or of what it loads) differs from the game's: copy it"""
		lp = norm(p)
		if lp not in self.reloc_cache:
			r = self.ri_modified(lp)
			if not r and (lp.endswith('.nif') or lp.endswith('.kfm')):
				r = any(isinstance(d, tuple) or self.ri_modified(d) for d in self.deps(lp))
			self.reloc_cache[lp] = r
		return self.reloc_cache[lp]

	def spelled(self, lp):
		"""a normalized path in the spelling of RI's or the game's folders"""
		d = os.path.dirname(lp)
		parts = []
		while d and d not in self.dir_case:
			parts.append(os.path.basename(d))
			d = os.path.dirname(d)
		head = self.dir_case.get(d, d)
		folder = '/'.join([head] + list(reversed(parts))) if head else '/'.join(reversed(parts))
		src = self.source(lp)
		name = os.path.basename(src[0].real(lp)) if src else os.path.basename(lp)
		return folder + '/' + name

	def dest_path(self, lp):
		if lp not in self.dest_case:
			real = self.spelled(lp)
			if lp.startswith(CITIES):
				self.dest_case[lp] = real[:len(CITIES)] + CITIES_SUB + real[len(CITIES):]
			else:
				assert lp.startswith('art/'), lp
				self.dest_case[lp] = DEST + real[4:]
		return self.dest_case[lp]

	def copy(self, src_path, dest):
		d = norm(dest)
		if d not in self.copies:
			src = self.source(src_path)
			self.copies[d] = (dest, src[0], norm(src_path))

	def resolve(self, p, force=False):
		"""the path to write in RI's art: RI's own files are copied to Art/RI, the game's are used where they are
		(force: copy anyway)"""
		if not p:
			return p
		if ',' in p:  # atlas button ",single.dds,atlas.dds,column,row"
			return ','.join(self.resolve(x) if norm(x).startswith('art/') else x for x in p.split(','))
		lp = norm(p)
		if '%1' in lp:  # wall pieces: wall%1_x.nif
			pat = re.escape(lp).replace('%1', '.*')
			hits = sorted(set(self.rif.match(pat) + self.base.match(pat)))
			if not hits:
				self.missing[p] += 1
				return p
			if not any(self.needs_reloc(h) for h in hits):
				return p
			for h in hits:  # all pieces in one folder
				self.resolve(h, True)
			d = self.dest_path(hits[0])
			return os.path.dirname(d) + '/' + basename(p.replace('\\', '/'))
		if lp not in self.rif and lp not in self.base:
			self.missing[p] += 1
			return p
		if not force and not self.needs_reloc(lp):
			return p
		dest = self.dest_path(lp)
		self.copy(lp, dest)
		ddir = os.path.dirname(dest)
		for d in self.deps(lp):
			if isinstance(d, tuple):
				self.copy(d[1], ddir + '/' + d[2])
			else:
				self.copy(d, ddir + '/' + os.path.basename(self.spelled(d)))
		return dest

	def port_art_define(self, el):
		e = copy.deepcopy(el)
		e.tail = None
		e.find('Type').text = PREFIX + text(el, 'Type')
		for tag in ('NIF', 'KFM', 'SHADERNIF', 'Button', 'ShadowDef/ShadowNIF', 'TrailDefinition/Texture'):
			x = e.find(tag)
			if x is not None and x.text and x.text.strip():
				x.text = self.resolve(x.text.strip())
		for tag in ('TrainSound', 'PatrolSound', 'SelectionSound', 'ActionSound',
				'AudioRunSounds/AudioRunTypeLoop', 'AudioRunSounds/AudioRunTypeEnd'):
			x = e.find(tag)
			if x is not None and x.text and x.text.strip() and x.text.strip() not in self.audio:
				self.unknown_sounds[x.text.strip()] += 1  # RI's own sounds aren't ported
				x.text = None
		return e

	# ----- city layout -----

	def bts_styles(self):
		return re.findall(r'<ArtStyleType>(\w+)</ArtStyleType>', open(self.bts_global_types, encoding='latin-1').read())

	def ri_styles(self):
		return re.findall(r'<ArtStyleType>(\w+)</ArtStyleType>', open(self.ri.global_types_path, encoding='latin-1').read())

	@staticmethod
	def style_set(value, all_styles):
		"""the art styles an ArtRef's ArtStyle attribute selects"""
		out = []
		for tok in [t.strip() for t in value.split(',') if t.strip()]:
			if tok == 'ARTSTYLE_ALL':
				out = list(all_styles)
			elif tok.startswith('!'):
				out = [s for s in out if s != tok[1:]]
			elif tok in all_styles and tok not in out:
				out.append(tok)
		return out

	def port_lsystem(self):
		"""K-Mod's city layout: BtS's, with BtS's houses and lots limited to BtS's art styles, plus RI's entries
		(styles, building layouts and file names prefixed or moved so that they never apply to BtS's cities)"""
		bts_styles = self.bts_styles()
		ri_styles = self.ri_styles()
		self.city_styles = ri_styles
		bts = open(self.bts_lsystem, encoding='latin-1').read()

		# 1. BtS's houses and lots: only in BtS's styles (RI's cities get RI's)
		def limit(m):
			block = m.group(0)
			if not re.match(r'<ArtRef Name="(generic|decoration):', block):
				return block
			st = re.search(r'<Attribute Class="ArtStyle">([^<]*)</Attribute>', block)
			if st:
				styles = self.style_set(st.group(1), bts_styles)
				return block[:st.start(1)] + ','.join(styles) + block[st.end(1):]
			indent = re.search(r'\n([ \t]*)<', block)
			ind = indent.group(1) if indent else '\t\t\t'
			first = block.index('>') + 1
			return block[:first] + '\n' + ind + '<Attribute Class="ArtStyle">%s</Attribute>' % ','.join(bts_styles) + block[first:]
		bts = re.sub(r'<ArtRef Name="[^"]*">.*?</ArtRef>', limit, bts, flags=re.S)

		# 2. RI's entries, node by node
		ri_root = parse_xml(self.ri.lsystem_path)
		ri_nodes = collections.defaultdict(list)
		for n in ri_root.findall('LNode'):
			ri_nodes[n.get('Name')].append(n)
		self.ri_lsystems = set()
		blocks = collections.defaultdict(list)
		for name, nodes in ri_nodes.items():
			for k, n in enumerate(nodes):
				refs = []
				for ref in n.findall('ArtRef'):
					r = self.port_artref(ref, ri_styles)
					if r is not None:
						refs.append(r)
				blocks[(name, k)] = refs
		seen = collections.Counter()
		out = []
		pos = 0
		for m in re.finditer(r'<LNode Name="([^"]*)">.*?(\n[ \t]*)</LNode>', bts, flags=re.S):
			name = m.group(1)
			k = seen[name]
			seen[name] += 1
			refs = blocks.pop((name, k), [])
			out.append(bts[pos:m.start(2)])
			if refs:
				out.append('\n\t\t<!-- Realism Invictus (only in its art styles and building layouts, prefixed RI_) -->')
				for r in refs:
					ET.indent(r, space='\t', level=2)
					out.append('\n\t\t' + ET.tostring(r, encoding='unicode').rstrip())
			pos = m.start(2)
		out.append(bts[pos:])
		for (name, k), refs in blocks.items():
			if refs:
				warn('RI city layout node %s (%d) has no BtS counterpart; %d entries left out' % (name, k, len(refs)))
		return ''.join(out)

	def port_artref(self, ref, ri_styles):
		r = copy.deepcopy(ref)
		r.tail = None
		kind, _, rest = r.get('Name').partition(':')
		if kind == 'building':
			self.ri_lsystems.add(rest)
			r.set('Name', 'building:' + PREFIX + rest)
		elif kind in ('generic', 'decoration'):
			path, sep, node = rest.partition('::')
			full = norm(CITIES + path)
			if full not in self.rif and full not in self.base:
				self.missing['%s (city)' % path] += 1
				return None
			if self.needs_reloc(full):
				dest = self.resolve(full)  # Art/Structures/Cities/RI/...
				path = dest[len(CITIES):]
			r.set('Name', kind + ':' + path + sep + node)
		else:
			warn('city layout entry %s left out' % r.get('Name'))
			return None
		st = None
		for a in r.findall('Attribute'):
			cls = a.get('Class')
			if cls == 'ArtStyle':
				st = a
			elif cls == 'Scalar' and a.text:
				key, _, val = a.text.partition(':')
				if key in ('szNIF', 'szWallArtFile', 'szPostArtFile') and val:
					n = len(self.missing)
					a.text = key + ':' + self.resolve(val.strip())
					if len(self.missing) > n:  # RI names a file it doesn't have
						return None
		styles = self.style_set(st.text or '', ri_styles) if st is not None else list(ri_styles)
		if not styles:
			return None
		if st is None and kind != 'building':
			st = ET.Element('Attribute', {'Class': 'ArtStyle'})
			r.insert(0, st)
		if st is not None:
			st.text = ','.join(PREFIX + s for s in styles)
		return r

	def port_global_types(self):
		s = open(self.bts_global_types, encoding='latin-1').read()
		i = s.index('</ArtStyleTypes>')
		j = s.rindex('\n', 0, i) + 1
		add = '\t\t<!-- Realism Invictus city styles (K-Mod-Magnus, option "Realism Invictus graphics") -->\n'
		add += ''.join('\t\t<ArtStyleType>%s%s</ArtStyleType>\n' % (PREFIX, st) for st in self.city_styles)
		return s[:j] + add + s[j:]

	def fix_lsystem(self, e, tag):
		"""<LSystem>: RI's building layout (prefixed), or the K-Mod building's where RI has none"""
		ls = e.find('LSystem')
		if ls is None:
			ls = ET.Element('LSystem')
			e.insert(1, ls)
		v = (ls.text or '').strip()
		if v and v in self.ri_lsystems:
			ls.text = PREFIX + v
			return
		km_ls = None
		for b, civs, t in self.building_specs:
			if t == tag:
				a = self.km.building_art.get(self.km.buildings[b]['art'])
				if a is not None:
					km_ls = text(a, 'LSystem')
					break
		ls.text = km_ls or 'LSYSTEM_2x2'

	# ----- output -----

	def run(self):
		mod = self.mod
		dry = self.args.dry_run
		self.port_units()
		self.port_buildings()
		lsystem = self.port_lsystem()  # first: building art defines need RI's layout names
		tags = []
		for unit, civs, spec in self.unit_specs:
			for req, tiers in spec['groups']:
				for t, tag in tiers:
					if tag not in tags:
						tags.append(tag)
		root = ET.Element('Civ4ArtDefines')
		infos = sub(root, 'UnitArtInfos')
		for tag in tags:
			infos.append(self.port_art_define(self.ri.unit_art[tag]))
		self.n_unit_art = len(tags)
		if not dry:
			write_xml(os.path.join(mod, 'Assets/xml/Art/RI_CIV4ArtDefines_Unit.xml'), root,
				'CIV4ArtDefinesSchema.xml', self.version)
		btags = []
		for b, civs, tag in self.building_specs:
			if tag not in btags:
				btags.append(tag)
		root = ET.Element('Civ4ArtDefines')
		infos = sub(root, 'BuildingArtInfos')
		for tag in btags:
			e = self.port_art_define(self.ri.building_art[tag])
			self.fix_lsystem(e, tag)
			infos.append(e)
		self.n_building_art = len(btags)
		if not dry:
			write_xml(os.path.join(mod, 'Assets/xml/Art/RI_CIV4ArtDefines_Building.xml'), root,
				'CIV4ArtDefinesSchema.xml', self.version)
			bdir = [d for d in os.listdir(os.path.join(mod, 'Assets/xml')) if d.lower() == 'buildings']
			write_text(os.path.join(mod, 'Assets/xml', bdir[0] if bdir else 'Buildings', 'CIV4CityLSystem.xml'), lsystem)
			write_text(os.path.join(mod, 'Assets/xml/GlobalTypes.xml'), self.port_global_types())
			self.write_infos()
			self.copy_files()
		if self.args.report:
			self.write_report(self.args.report)

	def write_infos(self):
		root = ET.Element('Civ4RIArtInfos')
		units = sub(root, 'RIUnitArtInfos')
		for unit, civs, spec in self.unit_specs:
			e = sub(units, 'RIUnitArtInfo')
			sub(e, 'UnitType', unit)
			ce = sub(e, 'Civilizations')
			for c in civs:
				sub(ce, 'CivilizationType', c)
			sub(e, 'fMaxSpeed', spec['speed'])
			sub(e, 'fPadTime', spec['pad'])
			mg = sub(e, 'UnitMeshGroups')
			for req, tiers in spec['groups']:
				g = sub(mg, 'UnitMeshGroup')
				sub(g, 'iRequired', str(req))
				for t, tag in tiers:
					sub(g, t + 'ArtDefineTag', PREFIX + tag)
		blds = sub(root, 'RIBuildingArtInfos')
		for b, civs, tag in self.building_specs:
			e = sub(blds, 'RIBuildingArtInfo')
			sub(e, 'BuildingType', b)
			ce = sub(e, 'Civilizations')
			for c in civs:
				sub(ce, 'CivilizationType', c)
			sub(e, 'ArtDefineTag', PREFIX + tag)
		civs = sub(root, 'RICivilizationArtInfos')
		for civ in self.km.civs:
			ri_civ = CIV_MAP.get(civ)
			style = self.ri.civs.get(ri_civ, {}).get('art_style') if ri_civ else None
			if style in self.city_styles:
				e = sub(civs, 'RICivilizationArtInfo')
				sub(e, 'CivilizationType', civ)
				sub(e, 'ArtStyleType', PREFIX + style)
		write_xml(os.path.join(self.mod, 'Assets/xml/Art/RI_CIV4ArtInfos.xml'), root, 'RI_CIV4ArtSchema.xml', self.version)
		write_text(os.path.join(self.mod, 'Assets/xml/Art/RI_CIV4ArtSchema.xml'), SCHEMA)

	def copy_files(self):
		root = os.path.join(self.mod, 'Assets')
		for old in (os.path.join(root, 'Art', 'RI'), os.path.join(root, 'Art', 'Structures', 'Cities', 'RI')):
			if os.path.isdir(old):
				shutil.rmtree(old)
		for d, (dest, fs, src) in sorted(self.copies.items()):
			out = os.path.join(root, dest)
			os.makedirs(os.path.dirname(out), exist_ok=True)
			with open(out, 'wb') as f:
				f.write(fs.read(src))

	def summary(self):
		n_base = sum(1 for d, (r, fs, s) in self.copies.items() if fs is self.base)
		size = sum(len(fs.read(s)) for d, (r, fs, s) in self.copies.items())
		print('units with RI art: %d of %d (%d entries)' % (
			len(set(u for u, c, s in self.unit_specs)), len(self.km.units), len(self.unit_specs)))
		print('buildings with RI art: %d of %d (%d entries)' % (
			len(set(b for b, c, t in self.building_specs)), len(self.km.buildings), len(self.building_specs)))
		print('unit art defines: %d, building art defines: %d' % (self.n_unit_art, self.n_building_art))
		print('files: %d (%.1f MB), %d of them the game\'s own, copied next to RI\'s' % (len(self.copies), size / 1e6, n_base))
		if self.missing:
			print('referenced by RI but in neither RI nor the game (left as they are): %d' % len(self.missing))
			for p, n in self.missing.most_common(30):
				print('   ', p)
		if self.unknown_sounds:
			print('RI sounds K-Mod lacks (left out): %s' % ', '.join(sorted(self.unknown_sounds)))

	def write_report(self, path):
		ri, km = self.ri, self.km
		lines = ['# Realism Invictus art in K-Mod-Magnus', '',
			'Generated by `tools/ri_art/port_ri_art.py` from Realism Invictus %s. For each K-Mod unit and building: the RI unit '
			'or building whose art it gets (by default, and for the civilizations that get another).' % self.version, '',
			'## Units', '', '| Unit | Default | Per civilization |', '|---|---|---|']
		for unit, default, per in self.unit_report:
			d = default['src'] if default else '(K-Mod art)'
			p = '; '.join('%s: %s' % (', '.join(c[13:].title() for c in civs), spec['src']) for civs, spec in per)
			lines.append('| %s | %s | %s |' % (unit, d, p))
		lines += ['', '## Buildings', '', '| Building | Default | Per civilization |', '|---|---|---|']
		for b, src, default, per in self.building_report:
			d = src if default else '(K-Mod art)'
			p = '; '.join('%s: %s' % (', '.join(c[13:].title() for c in civs), art) for art, civs in per)
			lines.append('| %s | %s | %s |' % (b, d, p))
		write_text(path, '\n'.join(lines) + '\n')


def main():
	ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
	ap.add_argument('--ri', required=True, help='folder that holds RI\'s Assets')
	ap.add_argument('--civ4', required=True, help='Civilization IV folder (holds Assets, Warlords, Beyond the Sword)')
	ap.add_argument('--mod', default=os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..'),
		help='K-Mod-Magnus repository (default: this script\'s)')
	ap.add_argument('--ri-version', help='RI version for the generated files\' header, if RI\'s ini has none')
	ap.add_argument('--dry-run', action='store_true', help='report only, write nothing')
	ap.add_argument('--report', help='write a table of what each unit and building gets (Markdown) here')
	args = ap.parse_args()
	p = Port(args)
	p.run()
	p.summary()


if __name__ == '__main__':
	main()

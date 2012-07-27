<?xml version="1.0" encoding="UTF-8"?>

<model formalismUrl="http://formalisms.cosyverif.org/pt-net.fml"
  xmlns="http://cosyverif.org/ns/model">

<attribute name="authors">Cesar Rodriguez</attribute>
<attribute name="title"></attribute>
<attribute name="date"></attribute>
<attribute name="note">Designed with Coloane. Not 1-safe!!</attribute>
<attribute name="version">0,0</attribute>

<node id="3" nodeType="place">
<attribute name="name">p4</attribute>
<attribute name="marking">0</attribute>
</node>
<node id="4" nodeType="place">
<attribute name="name">p1</attribute>
<attribute name="marking">1</attribute>
</node>
<node id="5" nodeType="place">
<attribute name="name">p2</attribute>
<attribute name="marking">1</attribute>
</node>
<node id="6" nodeType="place">
<attribute name="name">p3</attribute>
<attribute name="marking">0</attribute>
</node>
<node id="7" nodeType="transition">
<attribute name="name">t2</attribute>
</node>
<node id="8" nodeType="transition">
<attribute name="name">t1</attribute>
</node>
<node id="9" nodeType="transition">
<attribute name="name">t3</attribute>
</node>
<arc id="17" source="8" target="6" arcType="arc">
<attribute name="valuation">1</attribute>
</arc>
<arc id="16" source="4" target="8" arcType="arc">
<attribute name="valuation">1</attribute>
</arc>
<arc id="18" source="7" target="4" arcType="arc">
<attribute name="valuation">1</attribute>
</arc>
<arc id="10" source="6" target="9" arcType="readarc">
<attribute name="valuation">1</attribute>
</arc>
<arc id="11" source="9" target="3" arcType="arc">
<attribute name="valuation">1</attribute>
</arc>
<arc id="12" source="4" target="7" arcType="arc">
<attribute name="valuation">1</attribute>
</arc>
<arc id="13" source="7" target="6" arcType="arc">
<attribute name="valuation">1</attribute>
</arc>
<arc id="14" source="5" target="9" arcType="arc">
<attribute name="valuation">1</attribute>
</arc>
<arc id="15" source="9" target="4" arcType="arc">
<attribute name="valuation">1</attribute>
</arc>
</model>

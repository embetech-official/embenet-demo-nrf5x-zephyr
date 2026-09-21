<?xml version='1.0' encoding='UTF-8' standalone='yes' ?>
<tagfile doxygen_version="1.16.1" doxygen_gitid="669aeeefca743c148e2d935b3d3c69535c7491e6">
  <compound kind="file">
    <name>crc.h</name>
    <path>embetech/</path>
    <filename>crc_8h.html</filename>
    <includes id="crc16_8h" name="crc16.h" local="no" import="no" module="no" objc="no">embetech/crc16.h</includes>
    <includes id="crc32_8h" name="crc32.h" local="no" import="no" module="no" objc="no">embetech/crc32.h</includes>
    <includes id="crc8_8h" name="crc8.h" local="no" import="no" module="no" objc="no">embetech/crc8.h</includes>
    <member kind="function">
      <type>char const *</type>
      <name>CRC_GetVersionString</name>
      <anchorfile>crc_8h.html</anchorfile>
      <anchor>a93f2861372dbc36e2d4ce4a1f5965a10</anchor>
      <arglist>(void)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>crc16.h</name>
    <path>embetech/</path>
    <filename>crc16_8h.html</filename>
    <member kind="function">
      <type>uint16_t</type>
      <name>CRC16_CCITT_Init</name>
      <anchorfile>group__crc16.html</anchorfile>
      <anchor>gaa806472cbb131db8d597b0eec3add9b4</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>uint16_t</type>
      <name>CRC16_CCITT_UpdateUsingLut</name>
      <anchorfile>group__crc16.html</anchorfile>
      <anchor>ga47d775af8f1552ef5eaae78ac84035f2</anchor>
      <arglist>(void const *data, size_t data_size, uint16_t crc)</arglist>
    </member>
    <member kind="function">
      <type>uint16_t</type>
      <name>CRC16_CCITT_UpdateUsingFormula</name>
      <anchorfile>group__crc16.html</anchorfile>
      <anchor>ga7eb83117342b2a9434b62cfa84fc4576</anchor>
      <arglist>(void const *data, size_t data_size, uint16_t crc)</arglist>
    </member>
    <member kind="function">
      <type>uint16_t</type>
      <name>CRC16_CCITT_Finalize</name>
      <anchorfile>group__crc16.html</anchorfile>
      <anchor>ga8a7e0cba051918b2afdd5b539852430b</anchor>
      <arglist>(uint16_t crc)</arglist>
    </member>
    <member kind="function">
      <type>uint16_t</type>
      <name>CRC16_MODBUS_Init</name>
      <anchorfile>group__crc16.html</anchorfile>
      <anchor>ga26e382870ae066606334032e719e2b12</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>uint16_t</type>
      <name>CRC16_MODBUS_UpdateUsingLut</name>
      <anchorfile>group__crc16.html</anchorfile>
      <anchor>gadcb06254d2b5e1964023f2ea77962dc1</anchor>
      <arglist>(void const *data, size_t data_size, uint16_t crc)</arglist>
    </member>
    <member kind="function">
      <type>uint16_t</type>
      <name>CRC16_MODBUS_UpdateUsingFormula</name>
      <anchorfile>group__crc16.html</anchorfile>
      <anchor>ga75bfc5751fc4cad6c4400fad54d128f8</anchor>
      <arglist>(void const *data, size_t data_size, uint16_t crc)</arglist>
    </member>
    <member kind="function">
      <type>uint16_t</type>
      <name>CRC16_MODBUS_Finalize</name>
      <anchorfile>group__crc16.html</anchorfile>
      <anchor>ga3299563908a553e0c7dfd85d132aab46</anchor>
      <arglist>(uint16_t crc)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>crc32.h</name>
    <path>embetech/</path>
    <filename>crc32_8h.html</filename>
    <member kind="function">
      <type>uint32_t</type>
      <name>CRC32_CKSUM_Init</name>
      <anchorfile>group__crc32.html</anchorfile>
      <anchor>gab27dfaa4713446ba9324676f421943d0</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>uint32_t</type>
      <name>CRC32_CKSUM_UpdateUsingFormula</name>
      <anchorfile>group__crc32.html</anchorfile>
      <anchor>ga6f0f66169f62f4c6ba3370ae7698d6dc</anchor>
      <arglist>(void const *data, size_t data_size, uint32_t crc)</arglist>
    </member>
    <member kind="function">
      <type>uint32_t</type>
      <name>CRC32_CKSUM_UpdateUsingLut</name>
      <anchorfile>group__crc32.html</anchorfile>
      <anchor>gafe0ba6062f95541942fb1096bb229998</anchor>
      <arglist>(void const *data, size_t data_size, uint32_t crc)</arglist>
    </member>
    <member kind="function">
      <type>uint32_t</type>
      <name>CRC32_CKSUM_Finalize</name>
      <anchorfile>group__crc32.html</anchorfile>
      <anchor>ga61d3e4298649b4c51725bab432cd3c28</anchor>
      <arglist>(uint32_t crc)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>crc8.h</name>
    <path>embetech/</path>
    <filename>crc8_8h.html</filename>
    <member kind="function">
      <type>uint8_t</type>
      <name>CRC8_ITU_Init</name>
      <anchorfile>group__crc8.html</anchorfile>
      <anchor>ga99f58095fdd930603f1bdb63cf26a5dc</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>uint8_t</type>
      <name>CRC8_ITU_UpdateUsingFormula</name>
      <anchorfile>group__crc8.html</anchorfile>
      <anchor>ga14fc37fcf65c18482075c54796590c77</anchor>
      <arglist>(void const *data, size_t data_size, uint8_t crc)</arglist>
    </member>
    <member kind="function">
      <type>uint8_t</type>
      <name>CRC8_ITU_UpdateUsingLut</name>
      <anchorfile>group__crc8.html</anchorfile>
      <anchor>ga563552163cc0a08de7848732fa545d10</anchor>
      <arglist>(void const *data, size_t data_size, uint8_t crc)</arglist>
    </member>
    <member kind="function">
      <type>uint8_t</type>
      <name>CRC8_ITU_Finalize</name>
      <anchorfile>group__crc8.html</anchorfile>
      <anchor>ga30087419e0160aa7b7dff94007457dc4</anchor>
      <arglist>(uint8_t crc)</arglist>
    </member>
  </compound>
  <compound kind="group">
    <name>crc16</name>
    <title>CRC-16 calculation routines</title>
    <filename>group__crc16.html</filename>
  </compound>
  <compound kind="group">
    <name>crc32</name>
    <title>CRC-32 calculation routines</title>
    <filename>group__crc32.html</filename>
  </compound>
  <compound kind="group">
    <name>crc8</name>
    <title>CRC-8 calculation routines</title>
    <filename>group__crc8.html</filename>
  </compound>
  <compound kind="page">
    <name>index</name>
    <title>EMBETECH CRC library</title>
    <filename>index.html</filename>
    <docanchor file="index.html" title="EMBETECH CRC library">mainpage</docanchor>
  </compound>
</tagfile>

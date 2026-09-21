<?xml version='1.0' encoding='UTF-8' standalone='yes' ?>
<tagfile doxygen_version="1.16.1" doxygen_gitid="669aeeefca743c148e2d935b3d3c69535c7491e6">
  <compound kind="file">
    <name>ring_buffer.h</name>
    <path>embetech/</path>
    <filename>ring__buffer_8h.html</filename>
    <class kind="struct">RingBuffer</class>
    <member kind="function">
      <type>bool</type>
      <name>RingBuffer_Init</name>
      <anchorfile>group__ring__buffer.html</anchorfile>
      <anchor>ga53ca62183c43feb6be6042410642d50d</anchor>
      <arglist>(RingBuffer *instance, uint8_t *dataBuffer, size_t dataBufferSize)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>RingBuffer_Clear</name>
      <anchorfile>group__ring__buffer.html</anchorfile>
      <anchor>ga5f2758edf19359b68dc8b18c68895f78</anchor>
      <arglist>(RingBuffer *instance)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>RingBuffer_IsEmpty</name>
      <anchorfile>group__ring__buffer.html</anchorfile>
      <anchor>ga10b2b44024e6db571b8894c0085e1c26</anchor>
      <arglist>(RingBuffer const *instance)</arglist>
    </member>
    <member kind="function">
      <type>size_t</type>
      <name>RingBuffer_GetLen</name>
      <anchorfile>group__ring__buffer.html</anchorfile>
      <anchor>ga6c5dd7808a7fd504d875801e2a12e160</anchor>
      <arglist>(RingBuffer const *instance)</arglist>
    </member>
    <member kind="function">
      <type>size_t</type>
      <name>RingBuffer_GetCapacity</name>
      <anchorfile>group__ring__buffer.html</anchorfile>
      <anchor>gaf80012aed5bcc3a329d64a30abb19720</anchor>
      <arglist>(RingBuffer const *instance)</arglist>
    </member>
    <member kind="function">
      <type>size_t</type>
      <name>RingBuffer_GetSpace</name>
      <anchorfile>group__ring__buffer.html</anchorfile>
      <anchor>ga0e597c864f3081ca179f94765b178558</anchor>
      <arglist>(RingBuffer const *instance)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>RingBuffer_PutChar</name>
      <anchorfile>group__ring__buffer.html</anchorfile>
      <anchor>ga257cb49affef169770cabbcdb341f53f</anchor>
      <arglist>(RingBuffer *instance, uint8_t data)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>RingBuffer_GetChar</name>
      <anchorfile>group__ring__buffer.html</anchorfile>
      <anchor>gacbee0e605ddac8f0924407ad51eb08a6</anchor>
      <arglist>(RingBuffer *instance, uint8_t *readData)</arglist>
    </member>
    <member kind="function">
      <type>char const *</type>
      <name>RingBuffer_GetVersionString</name>
      <anchorfile>group__ring__buffer.html</anchorfile>
      <anchor>gaf0ff743a4b4e35f3d7c58bfb7fc3dac9</anchor>
      <arglist>(void)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>ring_buffer_16bit.h</name>
    <path>embetech/</path>
    <filename>ring__buffer__16bit_8h.html</filename>
    <class kind="struct">RingBuffer16Bit</class>
    <member kind="function">
      <type>bool</type>
      <name>RingBuffer16Bit_Init</name>
      <anchorfile>group__ring__buffer__16bit.html</anchorfile>
      <anchor>ga925348dbb704188cd9cf7b2612140430</anchor>
      <arglist>(RingBuffer16Bit *instance, uint16_t *dataBuffer, size_t dataBufferSize)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>RingBuffer16Bit_Clear</name>
      <anchorfile>group__ring__buffer__16bit.html</anchorfile>
      <anchor>ga4630872eeec388686752daccd273f205</anchor>
      <arglist>(RingBuffer16Bit *instance)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>RingBuffer16Bit_IsEmpty</name>
      <anchorfile>group__ring__buffer__16bit.html</anchorfile>
      <anchor>ga2ea0e7d195759562c90677eb02286e20</anchor>
      <arglist>(RingBuffer16Bit const *instance)</arglist>
    </member>
    <member kind="function">
      <type>size_t</type>
      <name>RingBuffer16Bit_GetLen</name>
      <anchorfile>group__ring__buffer__16bit.html</anchorfile>
      <anchor>gabb570030e928ed3e8f69fe1ba4aa28df</anchor>
      <arglist>(RingBuffer16Bit const *instance)</arglist>
    </member>
    <member kind="function">
      <type>size_t</type>
      <name>RingBuffer16Bit_GetCapacity</name>
      <anchorfile>group__ring__buffer__16bit.html</anchorfile>
      <anchor>gad89db85624ea2f21717e1c41db53b5b3</anchor>
      <arglist>(RingBuffer16Bit const *instance)</arglist>
    </member>
    <member kind="function">
      <type>size_t</type>
      <name>RingBuffer16Bit_GetSpace</name>
      <anchorfile>group__ring__buffer__16bit.html</anchorfile>
      <anchor>ga8c5776894e423f4c2b66407561c848b1</anchor>
      <arglist>(RingBuffer16Bit const *instance)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>RingBuffer16Bit_PutChar</name>
      <anchorfile>group__ring__buffer__16bit.html</anchorfile>
      <anchor>ga8fed133cfd2e8f25a559f19957923a25</anchor>
      <arglist>(RingBuffer16Bit *instance, uint16_t data)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>RingBuffer16Bit_GetChar</name>
      <anchorfile>group__ring__buffer__16bit.html</anchorfile>
      <anchor>ga0fde9025cc10bf5b0f3f7ae3e8e5c5bf</anchor>
      <arglist>(RingBuffer16Bit *instance, uint16_t *readData)</arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>RingBuffer</name>
    <filename>struct_ring_buffer.html</filename>
    <member kind="variable">
      <type>uint8_t *</type>
      <name>dataBuffer</name>
      <anchorfile>struct_ring_buffer.html</anchorfile>
      <anchor>a46e8b62a9b2e16edeaeab14c239d4dd2</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>size_t</type>
      <name>dataBufferSize</name>
      <anchorfile>struct_ring_buffer.html</anchorfile>
      <anchor>a726c360b18f890addb4d5dec56a633b2</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>size_t</type>
      <name>count</name>
      <anchorfile>struct_ring_buffer.html</anchorfile>
      <anchor>af2b7d80c5ce693b628c8b7a60de02796</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint8_t *</type>
      <name>head</name>
      <anchorfile>struct_ring_buffer.html</anchorfile>
      <anchor>a733194311b6249bf83294d4cf4402680</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint8_t *</type>
      <name>tail</name>
      <anchorfile>struct_ring_buffer.html</anchorfile>
      <anchor>ae6576672ab3a8e8670905f152d37e146</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>RingBuffer16Bit</name>
    <filename>struct_ring_buffer16_bit.html</filename>
    <member kind="variable">
      <type>uint16_t *</type>
      <name>dataBuffer</name>
      <anchorfile>struct_ring_buffer16_bit.html</anchorfile>
      <anchor>a98711fa80163c13b3714cd6dff7031b6</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>size_t</type>
      <name>dataBufferSize</name>
      <anchorfile>struct_ring_buffer16_bit.html</anchorfile>
      <anchor>a2658dbdb3195f88585c89882108a07a2</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>size_t</type>
      <name>count</name>
      <anchorfile>struct_ring_buffer16_bit.html</anchorfile>
      <anchor>a4ee77dc9b7358a83c499ad97b4ceba1c</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint16_t *</type>
      <name>head</name>
      <anchorfile>struct_ring_buffer16_bit.html</anchorfile>
      <anchor>a7c2e94df53485bac84e0a0d610965416</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint16_t *</type>
      <name>tail</name>
      <anchorfile>struct_ring_buffer16_bit.html</anchorfile>
      <anchor>afb9d2945958a4822e5d3bced02edaf03</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="group">
    <name>ring_buffer</name>
    <title>This module delivers a basic, general-purpose ring buffer.</title>
    <filename>group__ring__buffer.html</filename>
    <class kind="struct">RingBuffer</class>
  </compound>
  <compound kind="group">
    <name>ring_buffer_16bit</name>
    <title>This module delivers a basic, general-purpose ring buffer.</title>
    <filename>group__ring__buffer__16bit.html</filename>
    <class kind="struct">RingBuffer16Bit</class>
  </compound>
  <compound kind="page">
    <name>index</name>
    <title>EMBETECH Ring Buffer library</title>
    <filename>index.html</filename>
    <docanchor file="index.html" title="EMBETECH Ring Buffer library">mainpage</docanchor>
  </compound>
</tagfile>

<?xml version='1.0' encoding='UTF-8' standalone='yes' ?>
<tagfile doxygen_version="1.17.0" doxygen_gitid="65a43c0aba45cc23b3ca11b6b5334d4eea931726">
  <compound kind="file">
    <name>ipv6.h</name>
    <path>embenet/</path>
    <filename>ipv6_8h.html</filename>
    <class kind="struct">EMBENET_IPV6</class>
    <member kind="define">
      <type>#define</type>
      <name>EMBENET_EUI64_INVALID</name>
      <anchorfile>group__embenet__common.html</anchorfile>
      <anchor>gadc50245686f6227b3575324f3cae74bd</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>EMBENET_GROUPID_INVALID</name>
      <anchorfile>group__embenet__common.html</anchorfile>
      <anchor>ga9f6249e8bc088149d32bc5175db257b0</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>uint64_t</type>
      <name>EMBENET_NetworkPrefix</name>
      <anchorfile>group__embenet__common.html</anchorfile>
      <anchor>gaf5d7df5491e5d345861630bb902f045d</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>uint64_t</type>
      <name>EMBENET_EUI64</name>
      <anchorfile>group__embenet__common.html</anchorfile>
      <anchor>ga76a4593255d48bcb4c3dd5f05de4011b</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>uint16_t</type>
      <name>EMBENET_GroupId</name>
      <anchorfile>group__embenet__common.html</anchorfile>
      <anchor>ga4bce7b07f054ab24c4cd4555c3d34702</anchor>
      <arglist></arglist>
    </member>
    <member kind="function" static="yes">
      <type>static EMBENET_IPV6</type>
      <name>EMBENET_AssembleUnicastIpv6</name>
      <anchorfile>group__embenet__common.html</anchorfile>
      <anchor>gae12f8ee7bbb2fde7b31b5b40735233e2</anchor>
      <arglist>(EMBENET_NetworkPrefix nwkPrefix, EMBENET_EUI64 uid)</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static EMBENET_IPV6</type>
      <name>EMBENET_AssembleMulticastIpv6</name>
      <anchorfile>group__embenet__common.html</anchorfile>
      <anchor>gac53c3642d315958c12e03073fc8870ed</anchor>
      <arglist>(EMBENET_NetworkPrefix nwkPrefix, EMBENET_GroupId gid)</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static EMBENET_EUI64</type>
      <name>EMBENET_GetUidFromIpv6</name>
      <anchorfile>group__embenet__common.html</anchorfile>
      <anchor>ga71cbb1cef890b87ce08395d3f7c1a90d</anchor>
      <arglist>(EMBENET_IPV6 const *ipv6)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>udp.h</name>
    <path>embenet/</path>
    <filename>udp_8h.html</filename>
    <includes id="ipv6_8h" name="ipv6.h" local="no" import="no" module="no" objc="no">embenet/ipv6.h</includes>
    <class kind="struct">EMBENET_UDP_SocketDescriptor</class>
    <member kind="typedef">
      <type>void(*</type>
      <name>EMBENET_UDP_RxDataHandler</name>
      <anchorfile>group__embenet__udp__api.html</anchorfile>
      <anchor>ga4f0d8b52997d4acddd4afd886fdb276b</anchor>
      <arglist>)(EMBENET_UDP_SocketDescriptor const *socket, EMBENET_IPV6 const *sourceAddress, uint16_t sourcePort, void const *data, size_t dataSize)</arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>EMBENET_UDP_Traffic</name>
      <anchorfile>group__embenet__udp__api.html</anchorfile>
      <anchor>ga7ee28160c98ad47d1b7fe912679ee0cf</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>EMBENET_UDP_TRAFFIC_UNICAST</name>
      <anchorfile>group__embenet__udp__api.html</anchorfile>
      <anchor>gga7ee28160c98ad47d1b7fe912679ee0cfab3c3065939cacbdec8de309322b31eba</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>EMBENET_UDP_TRAFFIC_MULTICAST</name>
      <anchorfile>group__embenet__udp__api.html</anchorfile>
      <anchor>gga7ee28160c98ad47d1b7fe912679ee0cfa58fdcbd87599304bd425ba15a168eeaf</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>EMBENET_UDP_TRAFFIC_ALL</name>
      <anchorfile>group__embenet__udp__api.html</anchorfile>
      <anchor>gga7ee28160c98ad47d1b7fe912679ee0cfa17c0ec4ef1ff88f69a64667230f2b7b6</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>EMBENET_UDP_Result</name>
      <anchorfile>group__embenet__udp__api.html</anchorfile>
      <anchor>gab2688259b3dceb141a81efb37ad14a11</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>EMBENET_UDP_RESULT_OK</name>
      <anchorfile>group__embenet__udp__api.html</anchorfile>
      <anchor>ggab2688259b3dceb141a81efb37ad14a11a4874ba48df69137098647ffeff174701</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>EMBENET_UDP_RESULT_UNSPECIFIED_ERROR</name>
      <anchorfile>group__embenet__udp__api.html</anchorfile>
      <anchor>ggab2688259b3dceb141a81efb37ad14a11a803a7c788ed3f5ea9f94438dad724aa8</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>EMBENET_UDP_RESULT_INVALID_ARGUMENT</name>
      <anchorfile>group__embenet__udp__api.html</anchorfile>
      <anchor>ggab2688259b3dceb141a81efb37ad14a11a08e29712ccd5d5ad9f50b5460e7c3f43</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>EMBENET_UDP_RESULT_NOT_SYNCHRONIZED</name>
      <anchorfile>group__embenet__udp__api.html</anchorfile>
      <anchor>ggab2688259b3dceb141a81efb37ad14a11aee3e7732a21231f909f0266ecc2e5474</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>EMBENET_UDP_RESULT_MAX_DATA_SIZE_EXCEEDED</name>
      <anchorfile>group__embenet__udp__api.html</anchorfile>
      <anchor>ggab2688259b3dceb141a81efb37ad14a11a3b6f5c6e44e99156eb6150a3d4a2aed9</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>EMBENET_UDP_RESULT_QUEUE_FULL</name>
      <anchorfile>group__embenet__udp__api.html</anchorfile>
      <anchor>ggab2688259b3dceb141a81efb37ad14a11a663f3e8274d6cbfe2a370dcf54b08005</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>EMBENET_UDP_RESULT_FORWARDING_ERROR</name>
      <anchorfile>group__embenet__udp__api.html</anchorfile>
      <anchor>ggab2688259b3dceb141a81efb37ad14a11a104b9b04577e06aab5290ce05d0609bb</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>EMBENET_UDP_RESULT_SOCKET_UNREGISTERED</name>
      <anchorfile>group__embenet__udp__api.html</anchorfile>
      <anchor>ggab2688259b3dceb141a81efb37ad14a11acbd7cde8922be7f3b7ea00514e0c1e27</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>EMBENET_UDP_RESULT_ALREADY</name>
      <anchorfile>group__embenet__udp__api.html</anchorfile>
      <anchor>ggab2688259b3dceb141a81efb37ad14a11a7ca158f18155106d9a5b279739d48a05</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>EMBENET_UDP_Result</type>
      <name>EMBENET_UDP_RegisterSocket</name>
      <anchorfile>group__embenet__udp__api.html</anchorfile>
      <anchor>gaa96b953bd563394c6e91e0fa237cb636</anchor>
      <arglist>(EMBENET_UDP_SocketDescriptor *socket)</arglist>
    </member>
    <member kind="function">
      <type>EMBENET_UDP_Result</type>
      <name>EMBENET_UDP_UnregisterSocket</name>
      <anchorfile>group__embenet__udp__api.html</anchorfile>
      <anchor>gad80ff856bc6683b51344b400f6e713a6</anchor>
      <arglist>(EMBENET_UDP_SocketDescriptor *socket)</arglist>
    </member>
    <member kind="function">
      <type>size_t</type>
      <name>EMBENET_UDP_GetMaxDataSize</name>
      <anchorfile>group__embenet__udp__api.html</anchorfile>
      <anchor>ga2bfe0f184ddd703474d6980d2b49a86f</anchor>
      <arglist>(EMBENET_UDP_SocketDescriptor const *socket)</arglist>
    </member>
    <member kind="function">
      <type>EMBENET_UDP_Result</type>
      <name>EMBENET_UDP_Send</name>
      <anchorfile>group__embenet__udp__api.html</anchorfile>
      <anchor>ga88b8b572de981685ae99a376b2c3fe3a</anchor>
      <arglist>(EMBENET_UDP_SocketDescriptor const *socket, EMBENET_IPV6 const *destinationAddress, uint16_t destinationPort, void const *data, size_t dataSize)</arglist>
    </member>
    <member kind="function">
      <type>char const *</type>
      <name>EMBENET_UDP_GetVersionString</name>
      <anchorfile>group__embenet__udp__api.html</anchorfile>
      <anchor>ga1da8558d435746facc0c3ff4eab5c6db</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>EMBENET_UDP_IsOsDependent</name>
      <anchorfile>group__embenet__udp__api.html</anchorfile>
      <anchor>ga47d4bd418330ac567fc1da1e882a8669</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>EMBENET_SetSourceAddress</name>
      <anchorfile>group__embenet__udp__api.html</anchorfile>
      <anchor>gae75abeda6602297fc7083d7e95d79310</anchor>
      <arglist>(char const *addr)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>EMBENET_UDP_Proc</name>
      <anchorfile>group__embenet__udp__api.html</anchorfile>
      <anchor>gab5b82969fbf755ea9a8f54d00061eebc</anchor>
      <arglist>(void)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>ipv6_addressing.md</name>
    <path></path>
    <filename>ipv6__addressing_8md.html</filename>
  </compound>
  <compound kind="file">
    <name>porting.md</name>
    <path></path>
    <filename>porting_8md.html</filename>
  </compound>
  <compound kind="file">
    <name>readme.md</name>
    <path></path>
    <filename>readme_8md.html</filename>
  </compound>
  <compound kind="file">
    <name>using_udp_sockets.md</name>
    <path></path>
    <filename>using__udp__sockets_8md.html</filename>
  </compound>
  <compound kind="struct">
    <name>EMBENET_IPV6</name>
    <filename>struct_e_m_b_e_n_e_t___i_p_v6.html</filename>
    <member kind="variable">
      <type>uint8_t</type>
      <name>value</name>
      <anchorfile>struct_e_m_b_e_n_e_t___i_p_v6.html</anchorfile>
      <anchor>a6d7d3e6b98c69bbd21e7d5f7f0e90777</anchor>
      <arglist>[16]</arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>EMBENET_UDP_SocketDescriptor</name>
    <filename>struct_e_m_b_e_n_e_t___u_d_p___socket_descriptor.html</filename>
    <member kind="variable">
      <type>EMBENET_UDP_SocketDescriptor *</type>
      <name>next</name>
      <anchorfile>struct_e_m_b_e_n_e_t___u_d_p___socket_descriptor.html</anchorfile>
      <anchor>a413d5192b1c8825d9ab1da081653e205</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>EMBENET_UDP_RxDataHandler</type>
      <name>rxDataHandler</name>
      <anchorfile>struct_e_m_b_e_n_e_t___u_d_p___socket_descriptor.html</anchorfile>
      <anchor>a4e028d89a63462c51bd6deeacc3f2906</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>void *</type>
      <name>userContext</name>
      <anchorfile>struct_e_m_b_e_n_e_t___u_d_p___socket_descriptor.html</anchorfile>
      <anchor>aef8142621808e0b2a663d3796483f8fb</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint16_t</type>
      <name>port</name>
      <anchorfile>struct_e_m_b_e_n_e_t___u_d_p___socket_descriptor.html</anchorfile>
      <anchor>ad58b1c2a5850d234932a8dd4f699717a</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>EMBENET_GroupId</type>
      <name>groupId</name>
      <anchorfile>struct_e_m_b_e_n_e_t___u_d_p___socket_descriptor.html</anchorfile>
      <anchor>ac7e8f1c6a11a0b0e2c1697e10bd6d4a1</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>EMBENET_UDP_Traffic</type>
      <name>handledTraffic</name>
      <anchorfile>struct_e_m_b_e_n_e_t___u_d_p___socket_descriptor.html</anchorfile>
      <anchor>a16500658857e16d2c66c97455930a963</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="group">
    <name>embenet_common</name>
    <title>embeNET common definitions</title>
    <filename>group__embenet__common.html</filename>
    <class kind="struct">EMBENET_IPV6</class>
  </compound>
  <compound kind="group">
    <name>embenet_udp_api</name>
    <title>embeNET UDP C API</title>
    <filename>group__embenet__udp__api.html</filename>
    <class kind="struct">EMBENET_UDP_SocketDescriptor</class>
  </compound>
  <compound kind="page">
    <name>embenet_ipv6_addressing</name>
    <title>embeNET IPv6 addressing</title>
    <filename>embenet_ipv6_addressing.html</filename>
  </compound>
  <compound kind="page">
    <name>embenet_udp_porting</name>
    <title>Porting embeNET UDP</title>
    <filename>embenet_udp_porting.html</filename>
  </compound>
  <compound kind="page">
    <name>embenet_udp_using_sockets</name>
    <title>Using UDP sockets</title>
    <filename>embenet_udp_using_sockets.html</filename>
  </compound>
  <compound kind="dir">
    <name>embenet</name>
    <path>embenet/</path>
    <filename>dir_8af79a07e95c0b3929a81bd4a67f54dd.html</filename>
    <file>ipv6.h</file>
    <file>udp.h</file>
  </compound>
  <compound kind="page">
    <name>index</name>
    <title>embeNET UDP</title>
    <filename>index.html</filename>
    <subpage>embenet_ipv6_addressing.html</subpage>
    <subpage>embenet_udp_using_sockets.html</subpage>
    <subpage>embenet_udp_porting.html</subpage>
    <docanchor file="index.html" title="embeNET UDP">md_readme</docanchor>
  </compound>
</tagfile>

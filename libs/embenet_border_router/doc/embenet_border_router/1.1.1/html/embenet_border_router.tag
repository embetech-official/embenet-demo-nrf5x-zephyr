<?xml version='1.0' encoding='UTF-8' standalone='yes' ?>
<tagfile doxygen_version="1.17.0" doxygen_gitid="65a43c0aba45cc23b3ca11b6b5334d4eea931726">
  <compound kind="file">
    <name>border_router.h</name>
    <path>embenet/</path>
    <filename>border__router_8h.html</filename>
    <includes id="border__router__types_8h" name="border_router_types.h" local="no" import="no" module="no" objc="no">embenet/border_router_types.h</includes>
    <member kind="function">
      <type>EMBENET_BR_Capabilities</type>
      <name>EMBENET_BR_GetMaxCapabilities</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>gae21463b1f0c4e10dfa907ee15cc74298</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>EMBENET_BR_Capabilities</type>
      <name>EMBENET_BR_GetDefaultCapabilities</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>gabe721812874193352f2067045b3c6c27</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>EMBENET_BR_Result</type>
      <name>EMBENET_BR_Init</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>gae43f864a4f9b5819a8e2515bb7e7ce8f</anchor>
      <arglist>(EMBENET_BR_Capabilities capabilities)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>EMBENET_BR_Deinit</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>ga24dbd74ecb3eecd0f5f759a05c57782d</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>EMBENET_BR_Result</type>
      <name>EMBENET_BR_SubscribeToEvents</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>gace800e60455598a2667b417acffc2bf4</anchor>
      <arglist>(EMBENET_BR_EventHandlers event_handlers, void *context)</arglist>
    </member>
    <member kind="function">
      <type>EMBENET_BR_Result</type>
      <name>EMBENET_BR_Configure</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>ga53fd1c3a972a94b92ada169aa88707c1</anchor>
      <arglist>(char const *root_node_interface, EMBENET_BR_NetworkParameters const *config, char const *wan_interface)</arglist>
    </member>
    <member kind="function">
      <type>EMBENET_BR_Result</type>
      <name>EMBENET_BR_Up</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>gaa83d11741f19c76219f4208612d9e74e</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>EMBENET_BR_Result</type>
      <name>EMBENET_BR_Down</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>gadebfbb20d31d2025c3ffb5f768536ef7</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>EMBENET_BR_Result</type>
      <name>EMBENET_BR_Proc</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>ga378c55a957b67eb0359223f3e3c26069</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>EMBENET_IPV6</type>
      <name>EMBENET_BR_GetRootNodeAddress</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>gaf234d4789c343dec4a26672a8314033b</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>EMBENET_NetworkPrefix</type>
      <name>EMBENET_BR_GetNetworkPrefix</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>gac8d3ea0b767d40cb7ac473ef3a45c78a</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>EMBENET_IPV6</type>
      <name>EMBENET_BR_GetWanAddress</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>ga689712b3acfcdcb01a7f1f50b376a83a</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>size_t</type>
      <name>EMBENET_BR_GetNodesCapacity</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>gae3cd4373c320a814401e0b07a39685c5</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>size_t</type>
      <name>EMBENET_BR_GetNodesCount</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>ga510df8efe5c0ab5800f31194f5493d05</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>EMBENET_BR_NodesList</type>
      <name>EMBENET_BR_GetNodes</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>ga094dfb5a2f243ea59c1335e8c9852b58</anchor>
      <arglist>(EMBENET_EUI64 *nodes_buffer, size_t nodes_buffer_size, size_t offset, size_t count)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>EMBENET_BR_IsNodePresent</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>ga20a1b81236c18720378fee15cabb3b5f</anchor>
      <arglist>(EMBENET_EUI64 uid)</arglist>
    </member>
    <member kind="function">
      <type>EMBENET_BR_NodeDetails</type>
      <name>EMBENET_BR_GetNodeDetails</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>gac5fbde4a06d2e26753441a7cdef0c622</anchor>
      <arglist>(EMBENET_EUI64 uid)</arglist>
    </member>
    <member kind="function">
      <type>int32_t</type>
      <name>EMBENET_BR_GetNodesInactivityTimeout</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>ga92e4913f87233d1ac86db06406f67807</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>EMBENET_BR_Result</type>
      <name>EMBENET_BR_SetNodesInactivityTimeout</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>ga4cd4e5e16d275c1fe6282cb21be3de65</anchor>
      <arglist>(int32_t timeout_sec)</arglist>
    </member>
    <member kind="function">
      <type>size_t</type>
      <name>EMBENET_BR_GetMulticastGroupCapacity</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>ga2fcdf986d337b2c93030589e0782a012</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>size_t</type>
      <name>EMBENET_BR_GetMulticastGroupCount</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>ga1b839c8e8fe7311b453a2db6e8fea2d6</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>EMBENET_BR_GroupsList</type>
      <name>EMBENET_BR_GetMulticastGroups</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>ga912c430f7a603be9c3b6a81b4b01dda0</anchor>
      <arglist>(EMBENET_GroupId *groups_buffer, size_t groups_buffer_size, size_t offset, size_t count)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>EMBENET_BR_IsMulticastGroupRegistered</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>ga3ea0f3e79b16c48b48c0e5fe917ce123</anchor>
      <arglist>(EMBENET_GroupId gid)</arglist>
    </member>
    <member kind="function">
      <type>EMBENET_BR_Result</type>
      <name>EMBENET_BR_Send</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>ga065a39573f7d4729dc2b4f71ad9904f9</anchor>
      <arglist>(EMBENET_UDP_SocketDescriptor const *socket, EMBENET_IPV6 const *dst, uint16_t dst_port, void const *data, size_t data_len)</arglist>
    </member>
    <member kind="function">
      <type>EMBENET_BR_Result</type>
      <name>EMBENET_BR_RegisterSocket</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>ga564d7d8f214ea990bf0f4d658f8ebe5f</anchor>
      <arglist>(EMBENET_UDP_SocketDescriptor *socket)</arglist>
    </member>
    <member kind="function">
      <type>EMBENET_BR_Result</type>
      <name>EMBENET_BR_UnregisterSocket</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>ga4d8a03a8ecad03e62f5060e47e0a4fc2</anchor>
      <arglist>(EMBENET_UDP_SocketDescriptor *socket)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>EMBENET_BR_IsSocketRegistered</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>gaa90ee568050fbbb45a718feeddb1cce3</anchor>
      <arglist>(EMBENET_UDP_SocketDescriptor const *socket)</arglist>
    </member>
    <member kind="function">
      <type>size_t</type>
      <name>EMBENET_BR_GetMaxDatagramLength</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>ga9f51b498b2da037c1168ad71ac71843d</anchor>
      <arglist>(EMBENET_UDP_SocketDescriptor const *socket)</arglist>
    </member>
    <member kind="function">
      <type>size_t</type>
      <name>EMBENET_BR_GetJoinRulesCapacity</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>gaf3152d34114698cbfb7b5661bca73e36</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>size_t</type>
      <name>EMBENET_BR_GetJoinRulesCount</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>gafe72b95ffb522199323bf887999e4099</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>EMBENET_BR_JoinRulesList</type>
      <name>EMBENET_BR_GetJoinRules</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>ga33e39564c2f03a809187f10ccc1d58af</anchor>
      <arglist>(EMBENET_BR_JoinRule *rules_buffer, size_t rules_buffer_size, size_t offset, size_t count)</arglist>
    </member>
    <member kind="function">
      <type>EMBENET_BR_Result</type>
      <name>EMBENET_BR_AddJoinRule</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>ga1a95f6f0b721fbc7171b36f9c8b12dba</anchor>
      <arglist>(EMBENET_BR_JoinRule *rule)</arglist>
    </member>
    <member kind="function">
      <type>EMBENET_BR_Result</type>
      <name>EMBENET_BR_RemoveJoinRule</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>gac70eb84629bb819a6dd0b25be59774b0</anchor>
      <arglist>(EMBENET_BR_JoinRule *rule)</arglist>
    </member>
    <member kind="function">
      <type>EMBENET_BR_Result</type>
      <name>EMBENET_BR_ClearJoinRules</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>gaf54f2fd81b95ae567ae02c11251f8f2a</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>EMBENET_BR_JoinPolicy</type>
      <name>EMBENET_BR_GetJoinPolicy</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>ga941833d70fd667f886dbd96429d6c984</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>EMBENET_BR_Result</type>
      <name>EMBENET_BR_SetJoinPolicy</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>gabfdee9c42666f1a704e8f1e27b65d55c</anchor>
      <arglist>(EMBENET_BR_JoinPolicy const *policy)</arglist>
    </member>
    <member kind="function">
      <type>EMBENET_BR_Result</type>
      <name>EMBENET_BR_ConfigurePromiscuousMode</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>ga42893a355204d2332cb3c53a9185c798</anchor>
      <arglist>(bool enable)</arglist>
    </member>
    <member kind="function">
      <type>char const *</type>
      <name>EMBENET_BR_GetVersionString</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>gaeb1c594a4562e329ca9839320bda9ea1</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>EMBENET_BR_Capabilities</type>
      <name>EMBENET_BR_GetMaxCapabilities</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>gae21463b1f0c4e10dfa907ee15cc74298</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>EMBENET_BR_Capabilities</type>
      <name>EMBENET_BR_GetDefaultCapabilities</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>gabe721812874193352f2067045b3c6c27</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>EMBENET_BR_Result</type>
      <name>EMBENET_BR_Init</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>gae43f864a4f9b5819a8e2515bb7e7ce8f</anchor>
      <arglist>(EMBENET_BR_Capabilities capabilities)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>EMBENET_BR_Deinit</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>ga24dbd74ecb3eecd0f5f759a05c57782d</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>EMBENET_BR_Result</type>
      <name>EMBENET_BR_SubscribeToEvents</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>gace800e60455598a2667b417acffc2bf4</anchor>
      <arglist>(EMBENET_BR_EventHandlers event_handlers, void *context)</arglist>
    </member>
    <member kind="function">
      <type>EMBENET_BR_Result</type>
      <name>EMBENET_BR_Configure</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>ga53fd1c3a972a94b92ada169aa88707c1</anchor>
      <arglist>(char const *root_node_interface, EMBENET_BR_NetworkParameters const *config, char const *wan_interface)</arglist>
    </member>
    <member kind="function">
      <type>EMBENET_BR_Result</type>
      <name>EMBENET_BR_Up</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>gaa83d11741f19c76219f4208612d9e74e</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>EMBENET_BR_Result</type>
      <name>EMBENET_BR_Down</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>gadebfbb20d31d2025c3ffb5f768536ef7</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>EMBENET_BR_Result</type>
      <name>EMBENET_BR_Proc</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>ga378c55a957b67eb0359223f3e3c26069</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>EMBENET_IPV6</type>
      <name>EMBENET_BR_GetRootNodeAddress</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>gaf234d4789c343dec4a26672a8314033b</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>EMBENET_NetworkPrefix</type>
      <name>EMBENET_BR_GetNetworkPrefix</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>gac8d3ea0b767d40cb7ac473ef3a45c78a</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>EMBENET_IPV6</type>
      <name>EMBENET_BR_GetWanAddress</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>ga689712b3acfcdcb01a7f1f50b376a83a</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>size_t</type>
      <name>EMBENET_BR_GetNodesCapacity</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>gae3cd4373c320a814401e0b07a39685c5</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>size_t</type>
      <name>EMBENET_BR_GetNodesCount</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>ga510df8efe5c0ab5800f31194f5493d05</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>EMBENET_BR_NodesList</type>
      <name>EMBENET_BR_GetNodes</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>ga094dfb5a2f243ea59c1335e8c9852b58</anchor>
      <arglist>(EMBENET_EUI64 *nodes_buffer, size_t nodes_buffer_size, size_t offset, size_t count)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>EMBENET_BR_IsNodePresent</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>ga20a1b81236c18720378fee15cabb3b5f</anchor>
      <arglist>(EMBENET_EUI64 uid)</arglist>
    </member>
    <member kind="function">
      <type>EMBENET_BR_NodeDetails</type>
      <name>EMBENET_BR_GetNodeDetails</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>gac5fbde4a06d2e26753441a7cdef0c622</anchor>
      <arglist>(EMBENET_EUI64 uid)</arglist>
    </member>
    <member kind="function">
      <type>int32_t</type>
      <name>EMBENET_BR_GetNodesInactivityTimeout</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>ga92e4913f87233d1ac86db06406f67807</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>EMBENET_BR_Result</type>
      <name>EMBENET_BR_SetNodesInactivityTimeout</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>ga4cd4e5e16d275c1fe6282cb21be3de65</anchor>
      <arglist>(int32_t timeout_sec)</arglist>
    </member>
    <member kind="function">
      <type>size_t</type>
      <name>EMBENET_BR_GetMulticastGroupCapacity</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>ga2fcdf986d337b2c93030589e0782a012</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>size_t</type>
      <name>EMBENET_BR_GetMulticastGroupCount</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>ga1b839c8e8fe7311b453a2db6e8fea2d6</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>EMBENET_BR_GroupsList</type>
      <name>EMBENET_BR_GetMulticastGroups</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>ga912c430f7a603be9c3b6a81b4b01dda0</anchor>
      <arglist>(EMBENET_GroupId *groups_buffer, size_t groups_buffer_size, size_t offset, size_t count)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>EMBENET_BR_IsMulticastGroupRegistered</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>ga3ea0f3e79b16c48b48c0e5fe917ce123</anchor>
      <arglist>(EMBENET_GroupId gid)</arglist>
    </member>
    <member kind="function">
      <type>EMBENET_BR_Result</type>
      <name>EMBENET_BR_Send</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>ga065a39573f7d4729dc2b4f71ad9904f9</anchor>
      <arglist>(EMBENET_UDP_SocketDescriptor const *socket, EMBENET_IPV6 const *dst, uint16_t dst_port, void const *data, size_t data_len)</arglist>
    </member>
    <member kind="function">
      <type>EMBENET_BR_Result</type>
      <name>EMBENET_BR_RegisterSocket</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>ga564d7d8f214ea990bf0f4d658f8ebe5f</anchor>
      <arglist>(EMBENET_UDP_SocketDescriptor *socket)</arglist>
    </member>
    <member kind="function">
      <type>EMBENET_BR_Result</type>
      <name>EMBENET_BR_UnregisterSocket</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>ga4d8a03a8ecad03e62f5060e47e0a4fc2</anchor>
      <arglist>(EMBENET_UDP_SocketDescriptor *socket)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>EMBENET_BR_IsSocketRegistered</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>gaa90ee568050fbbb45a718feeddb1cce3</anchor>
      <arglist>(EMBENET_UDP_SocketDescriptor const *socket)</arglist>
    </member>
    <member kind="function">
      <type>size_t</type>
      <name>EMBENET_BR_GetMaxDatagramLength</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>ga9f51b498b2da037c1168ad71ac71843d</anchor>
      <arglist>(EMBENET_UDP_SocketDescriptor const *socket)</arglist>
    </member>
    <member kind="function">
      <type>size_t</type>
      <name>EMBENET_BR_GetJoinRulesCapacity</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>gaf3152d34114698cbfb7b5661bca73e36</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>size_t</type>
      <name>EMBENET_BR_GetJoinRulesCount</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>gafe72b95ffb522199323bf887999e4099</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>EMBENET_BR_JoinRulesList</type>
      <name>EMBENET_BR_GetJoinRules</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>ga33e39564c2f03a809187f10ccc1d58af</anchor>
      <arglist>(EMBENET_BR_JoinRule *rules_buffer, size_t rules_buffer_size, size_t offset, size_t count)</arglist>
    </member>
    <member kind="function">
      <type>EMBENET_BR_Result</type>
      <name>EMBENET_BR_AddJoinRule</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>ga1a95f6f0b721fbc7171b36f9c8b12dba</anchor>
      <arglist>(EMBENET_BR_JoinRule *rule)</arglist>
    </member>
    <member kind="function">
      <type>EMBENET_BR_Result</type>
      <name>EMBENET_BR_RemoveJoinRule</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>gac70eb84629bb819a6dd0b25be59774b0</anchor>
      <arglist>(EMBENET_BR_JoinRule *rule)</arglist>
    </member>
    <member kind="function">
      <type>EMBENET_BR_Result</type>
      <name>EMBENET_BR_ClearJoinRules</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>gaf54f2fd81b95ae567ae02c11251f8f2a</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>EMBENET_BR_JoinPolicy</type>
      <name>EMBENET_BR_GetJoinPolicy</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>ga941833d70fd667f886dbd96429d6c984</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>EMBENET_BR_Result</type>
      <name>EMBENET_BR_SetJoinPolicy</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>gabfdee9c42666f1a704e8f1e27b65d55c</anchor>
      <arglist>(EMBENET_BR_JoinPolicy const *policy)</arglist>
    </member>
    <member kind="function">
      <type>EMBENET_BR_Result</type>
      <name>EMBENET_BR_ConfigurePromiscuousMode</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>ga42893a355204d2332cb3c53a9185c798</anchor>
      <arglist>(bool enable)</arglist>
    </member>
    <member kind="function">
      <type>char const *</type>
      <name>EMBENET_BR_GetVersionString</name>
      <anchorfile>group__embenet__border__router__c__api.html</anchorfile>
      <anchor>gaeb1c594a4562e329ca9839320bda9ea1</anchor>
      <arglist>(void)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>border_router.hpp</name>
    <path>embenet/</path>
    <filename>border__router_8hpp.html</filename>
    <includes id="border__router__types_8hpp" name="border_router_types.hpp" local="no" import="no" module="no" objc="no">embenet/border_router_types.hpp</includes>
    <class kind="class">embenet::border_router::BorderRouter</class>
    <namespace>embenet</namespace>
    <namespace>embenet::border_router</namespace>
  </compound>
  <compound kind="file">
    <name>border_router_types.h</name>
    <path>embenet/</path>
    <filename>border__router__types_8h.html</filename>
    <class kind="struct">EMBENET_BR_CryptoKey</class>
    <class kind="struct">EMBENET_BR_EventHandlers</class>
    <class kind="struct">EMBENET_BR_NetworkParameters</class>
    <class kind="struct">EMBENET_BR_Capabilities</class>
    <class kind="struct">EMBENET_BR_JoinRule</class>
    <class kind="struct">EMBENET_BR_JoinPolicy</class>
    <class kind="struct">EMBENET_BR_NodesList</class>
    <class kind="struct">EMBENET_BR_GroupsList</class>
    <class kind="struct">EMBENET_BR_JoinRulesList</class>
    <class kind="struct">EMBENET_BR_NodeDetails</class>
    <member kind="typedef">
      <type>int</type>
      <name>EMBENET_BR_Result</name>
      <anchorfile>group__embenet__common.html</anchorfile>
      <anchor>gad44c757dacfc86ac17a3b5602d08c3ef</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>uint16_t</type>
      <name>EMBENET_PANID</name>
      <anchorfile>group__embenet__common.html</anchorfile>
      <anchor>ga62e6aadfe9a9ce0c48d06f100408b3d9</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>uint64_t</type>
      <name>EMBENET_PAN_Prefix</name>
      <anchorfile>group__embenet__common.html</anchorfile>
      <anchor>ga5e9fa82a0b20114d2cf3f2046ef159bb</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>void(*</type>
      <name>EMBENET_BR_OnNetworkStarted</name>
      <anchorfile>group__embenet__common.html</anchorfile>
      <anchor>ga5e485923745375807b19eac9c6e7dfb6</anchor>
      <arglist>)(uint64_t root_node_uid, void *context)</arglist>
    </member>
    <member kind="typedef">
      <type>void(*</type>
      <name>EMBENET_BR_OnNetworkStopped</name>
      <anchorfile>group__embenet__common.html</anchorfile>
      <anchor>gaf0a623e75b6f228c8f0024667c31b482</anchor>
      <arglist>)(void *context)</arglist>
    </member>
    <member kind="typedef">
      <type>void(*</type>
      <name>EMBENET_BR_OnNodeJoined</name>
      <anchorfile>group__embenet__common.html</anchorfile>
      <anchor>ga28a90bd887bbaa7bc75492d221bb71bd</anchor>
      <arglist>)(EMBENET_EUI64 uid, void *context)</arglist>
    </member>
    <member kind="typedef">
      <type>void(*</type>
      <name>EMBENET_BR_OnNodeLeft</name>
      <anchorfile>group__embenet__common.html</anchorfile>
      <anchor>gaf7cff83c7517a4f2b5a402757b110476</anchor>
      <arglist>)(EMBENET_EUI64 uid, void *context)</arglist>
    </member>
    <member kind="typedef">
      <type>void(*</type>
      <name>EMBENET_BR_OnAuthenticationAttempt</name>
      <anchorfile>group__embenet__common.html</anchorfile>
      <anchor>ga7e5894ae458d6952be3b2f87b5dda2b0</anchor>
      <arglist>)(EMBENET_EUI64 uid, EMBENET_BR_AuthResult result, void *context)</arglist>
    </member>
    <member kind="typedef">
      <type>void(*</type>
      <name>EMBENET_BR_OnNodeAddedToGroup</name>
      <anchorfile>group__embenet__common.html</anchorfile>
      <anchor>ga8ea06361f20fddf2c3c28e53a50539de</anchor>
      <arglist>)(EMBENET_GroupId group_id, EMBENET_EUI64 uid, void *context)</arglist>
    </member>
    <member kind="typedef">
      <type>void(*</type>
      <name>EMBENET_BR_OnNodeRemovedFromGroup</name>
      <anchorfile>group__embenet__common.html</anchorfile>
      <anchor>gacc398266b80e72eeed47bcd55af64b11</anchor>
      <arglist>)(EMBENET_GroupId group_id, EMBENET_EUI64 uid, void *context)</arglist>
    </member>
    <member kind="typedef">
      <type>void(*</type>
      <name>EMBENET_BR_OnDataOnUnregisteredPort</name>
      <anchorfile>group__embenet__common.html</anchorfile>
      <anchor>ga611ef59650905173849ef8ad84604609</anchor>
      <arglist>)(uint16_t destination_port, uint16_t source_port, void *context)</arglist>
    </member>
    <member kind="typedef">
      <type>void(*</type>
      <name>EMBENET_BR_OnPacketDropped</name>
      <anchorfile>group__embenet__common.html</anchorfile>
      <anchor>gace950575002f44330d0f82b4340a3c7a</anchor>
      <arglist>)(uint8_t protocol, void *context)</arglist>
    </member>
    <member kind="typedef">
      <type>void(*</type>
      <name>EMBENET_BR_OnAsnUpdate</name>
      <anchorfile>group__embenet__common.html</anchorfile>
      <anchor>ga4c97f8aa990c9f6b138fbc53936dcd03</anchor>
      <arglist>)(uint64_t asn, void *context)</arglist>
    </member>
    <member kind="typedef">
      <type>void(*</type>
      <name>EMBENET_BR_OnRootNodeConnectionFailed</name>
      <anchorfile>group__embenet__common.html</anchorfile>
      <anchor>gabb28f1e032e4c988f609ec3cd94dcef6</anchor>
      <arglist>)(void *context)</arglist>
    </member>
    <member kind="typedef">
      <type>void(*</type>
      <name>EMBENET_BR_OnRootNodeIdentified</name>
      <anchorfile>group__embenet__common.html</anchorfile>
      <anchor>gafe4d1c798154375c5253aa6fb46ef74e</anchor>
      <arglist>)(uint64_t root_node_uid, void *context)</arglist>
    </member>
    <member kind="enumvalue">
      <name>EMBENET_BR_CRYPTO_KEY_LENGTH</name>
      <anchorfile>group__embenet__common.html</anchorfile>
      <anchor>ggab0f72943c937cc260aafe4bb6a1b6b34a565664dcb55003869080d7aefd2c1b4f</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>EMBENET_BR_MAX_PAN_DATA_SIZE</name>
      <anchorfile>group__embenet__common.html</anchorfile>
      <anchor>ggab0f72943c937cc260aafe4bb6a1b6b34af13483141f7a4e5bf8e108cea1ecad65</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>EMBENET_BR_MAX_NODE_MULTICAST_GROUPS</name>
      <anchorfile>group__embenet__common.html</anchorfile>
      <anchor>ggab0f72943c937cc260aafe4bb6a1b6b34a51c85f6e702973dbe21eddeaddcc0944</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>EMBENET_BR_AuthResult</name>
      <anchorfile>group__embenet__common.html</anchorfile>
      <anchor>gaa8f01431a0615d4bc39d3b2959f4c0f8</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>EMBENET_BR_AUTH_RESULT_AUTHENTICATED</name>
      <anchorfile>group__embenet__common.html</anchorfile>
      <anchor>ggaa8f01431a0615d4bc39d3b2959f4c0f8a265f734a5d0ad32f361a63e955297bee</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>EMBENET_BR_AUTH_RESULT_REJECTED</name>
      <anchorfile>group__embenet__common.html</anchorfile>
      <anchor>ggaa8f01431a0615d4bc39d3b2959f4c0f8a8bb7786bb04bba8c0dcebe6bba474114</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>EMBENET_BR_AUTH_RESULT_NO_MATCHING_RULE</name>
      <anchorfile>group__embenet__common.html</anchorfile>
      <anchor>ggaa8f01431a0615d4bc39d3b2959f4c0f8a3fb3386df5193827b37e44136b007106</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>border_router_types.hpp</name>
    <path>embenet/</path>
    <filename>border__router__types_8hpp.html</filename>
    <includes id="border__router__types_8h" name="border_router_types.h" local="no" import="no" module="no" objc="no">embenet/border_router_types.h</includes>
    <class kind="struct">embenet::border_router::Event</class>
    <class kind="struct">embenet::border_router::events::RootNodeIdentified</class>
    <class kind="struct">embenet::border_router::events::NetworkStarted</class>
    <class kind="struct">embenet::border_router::events::NetworkStopped</class>
    <class kind="struct">embenet::border_router::events::NodeJoinedNetwork</class>
    <class kind="struct">embenet::border_router::events::NodeLeftNetwork</class>
    <class kind="struct">embenet::border_router::events::AuthenticationAttempt</class>
    <class kind="struct">embenet::border_router::events::NodeJoinedGroup</class>
    <class kind="struct">embenet::border_router::events::NodeLeftGroup</class>
    <class kind="struct">embenet::border_router::events::DataOnUnregisteredPort</class>
    <class kind="struct">embenet::border_router::events::PacketDropped</class>
    <class kind="struct">embenet::border_router::events::AsnUpdate</class>
    <class kind="struct">embenet::border_router::events::RootNodeConnectionFailed</class>
    <namespace>embenet</namespace>
    <namespace>embenet::border_router</namespace>
    <namespace>embenet::border_router::events</namespace>
  </compound>
  <compound kind="file">
    <name>clock.h</name>
    <path>embenet_port/</path>
    <filename>clock_8h.html</filename>
    <member kind="function">
      <type>int64_t</type>
      <name>EMBENET_BR_PORT_GetTime</name>
      <anchorfile>clock_8h.html</anchorfile>
      <anchor>a67d2f82c1520e0d6fb4aa4cca4c3c03b</anchor>
      <arglist>(void)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>condition_variable.h</name>
    <path>embenet_port/</path>
    <filename>condition__variable_8h.html</filename>
    <includes id="mutex_8h" name="mutex.h" local="no" import="no" module="no" objc="no">embenet_port/mutex.h</includes>
    <member kind="typedef">
      <type>struct EMBENET_BR_PORT_ConditionVariable</type>
      <name>EMBENET_BR_PORT_ConditionVariable</name>
      <anchorfile>condition__variable_8h.html</anchorfile>
      <anchor>a0f923beca3c370c845e9f4184a736551</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>EMBENET_BR_PORT_ConditionVariable *</type>
      <name>EMBENET_BR_PORT_ConditionVariableCreate</name>
      <anchorfile>condition__variable_8h.html</anchorfile>
      <anchor>a3ad80b651f40ba796ab135ca343816f1</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>EMBENET_BR_PORT_ConditionVariableDestroy</name>
      <anchorfile>condition__variable_8h.html</anchorfile>
      <anchor>adc0433ca4776433d2d304750f66da5a1</anchor>
      <arglist>(EMBENET_BR_PORT_ConditionVariable *condition_variable)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>EMBENET_BR_PORT_ConditionVariableNotifyOne</name>
      <anchorfile>condition__variable_8h.html</anchorfile>
      <anchor>ab2f11a43df769d6271644cac45ead22a</anchor>
      <arglist>(EMBENET_BR_PORT_ConditionVariable *condition_variable)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>EMBENET_BR_PORT_ConditionVariableWaitFor</name>
      <anchorfile>condition__variable_8h.html</anchorfile>
      <anchor>a38a82cc8966d162edb5e0548cfcedae9</anchor>
      <arglist>(EMBENET_BR_PORT_ConditionVariable *condition_variable, EMBENET_BR_PORT_Mutex *mutex, int64_t timeout_ms)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>memory.h</name>
    <path>embenet_port/</path>
    <filename>memory_8h.html</filename>
    <member kind="function">
      <type>void *</type>
      <name>EMBENET_BR_PORT_Allocate</name>
      <anchorfile>memory_8h.html</anchorfile>
      <anchor>a651d25d1125330aec91ab1d37ac8a155</anchor>
      <arglist>(size_t size)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>EMBENET_BR_PORT_Deallocate</name>
      <anchorfile>memory_8h.html</anchorfile>
      <anchor>a946b770f1082bc3306280e6950843369</anchor>
      <arglist>(void *ptr)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>mutex.h</name>
    <path>embenet_port/</path>
    <filename>mutex_8h.html</filename>
    <member kind="typedef">
      <type>struct EMBENET_BR_PORT_Mutex</type>
      <name>EMBENET_BR_PORT_Mutex</name>
      <anchorfile>mutex_8h.html</anchorfile>
      <anchor>a1fa7d6a40ef6748ae53ed7b0a24c277d</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>EMBENET_BR_PORT_Mutex *</type>
      <name>EMBENET_BR_PORT_MutexCreate</name>
      <anchorfile>mutex_8h.html</anchorfile>
      <anchor>ae6281ea1952ecab3e7cb723fb72b9854</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>EMBENET_BR_PORT_MutexDestroy</name>
      <anchorfile>mutex_8h.html</anchorfile>
      <anchor>a4bea34a3be349e691462bfda62a2f716</anchor>
      <arglist>(EMBENET_BR_PORT_Mutex *mutex)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>EMBENET_BR_PORT_MutexLock</name>
      <anchorfile>mutex_8h.html</anchorfile>
      <anchor>a47e3db1dc68cf3f51648195191c89992</anchor>
      <arglist>(EMBENET_BR_PORT_Mutex *mutex)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>EMBENET_BR_PORT_MutexUnlock</name>
      <anchorfile>mutex_8h.html</anchorfile>
      <anchor>a359353d854ce90c01228b4048a8def29</anchor>
      <arglist>(EMBENET_BR_PORT_Mutex *mutex)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>EMBENET_BR_PORT_MutexTryLock</name>
      <anchorfile>mutex_8h.html</anchorfile>
      <anchor>ad6c910c4b792527e9685500e7b2b3968</anchor>
      <arglist>(EMBENET_BR_PORT_Mutex *mutex)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>network_adapter.h</name>
    <path>embenet_port/</path>
    <filename>network__adapter_8h.html</filename>
    <member kind="typedef">
      <type>struct EMBENET_BR_PORT_NetworkAdapter</type>
      <name>EMBENET_BR_PORT_NetworkAdapter</name>
      <anchorfile>network__adapter_8h.html</anchorfile>
      <anchor>a912fddb9336be664326e05a9a6214190</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>EMBENET_BR_PORT_NetworkAdapter *</type>
      <name>EMBENET_BR_PORT_NetworkAdapterCreate</name>
      <anchorfile>network__adapter_8h.html</anchorfile>
      <anchor>adb0ae62a12652b8d7eef6e801c65ed74</anchor>
      <arglist>(char const *key)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>EMBENET_BR_PORT_NetworkAdapterDestroy</name>
      <anchorfile>network__adapter_8h.html</anchorfile>
      <anchor>a93a53192b02cc536180b724709cefb48</anchor>
      <arglist>(EMBENET_BR_PORT_NetworkAdapter *adapter)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>EMBENET_BR_PORT_NetworkAdapterSetDataReceivedCallback</name>
      <anchorfile>network__adapter_8h.html</anchorfile>
      <anchor>a9a703bfabedf64171f8dbfdf794df1dc</anchor>
      <arglist>(EMBENET_BR_PORT_NetworkAdapter *adapter, void(*callback)(void *context, void const *data, size_t data_length), void *context)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>EMBENET_BR_PORT_NetworkAdapterSetLinkStateChangedCallback</name>
      <anchorfile>network__adapter_8h.html</anchorfile>
      <anchor>a1e4165cb44950a7a8ae3f0d9d4de85c0</anchor>
      <arglist>(EMBENET_BR_PORT_NetworkAdapter *adapter, void(*callback)(void *context, bool is_up), void *context)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>EMBENET_BR_PORT_NetworkAdapterUp</name>
      <anchorfile>network__adapter_8h.html</anchorfile>
      <anchor>aa3378d8bd467c570fa1f075d9c80079d</anchor>
      <arglist>(EMBENET_BR_PORT_NetworkAdapter *adapter)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>EMBENET_BR_PORT_NetworkAdapterDown</name>
      <anchorfile>network__adapter_8h.html</anchorfile>
      <anchor>a87e609c89c2f1ceaadec4976b088dd3f</anchor>
      <arglist>(EMBENET_BR_PORT_NetworkAdapter *adapter)</arglist>
    </member>
    <member kind="function">
      <type>char const *</type>
      <name>EMBENET_BR_PORT_NetworkAdapterGetName</name>
      <anchorfile>network__adapter_8h.html</anchorfile>
      <anchor>af2932a74513d559ea1c4613ddde70740</anchor>
      <arglist>(EMBENET_BR_PORT_NetworkAdapter const *adapter)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>EMBENET_BR_PORT_NetworkAdapterGetAddress</name>
      <anchorfile>network__adapter_8h.html</anchorfile>
      <anchor>a7e07a04fee85d9d786a0135d1b2cc0d5</anchor>
      <arglist>(EMBENET_BR_PORT_NetworkAdapter const *adapter, uint8_t *address_buffer)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>EMBENET_BR_PORT_NetworkAdapterSetAddress</name>
      <anchorfile>network__adapter_8h.html</anchorfile>
      <anchor>aaaaa86d9281dce58dd5edb42acb2ee8b</anchor>
      <arglist>(EMBENET_BR_PORT_NetworkAdapter *adapter, uint8_t const *address)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>EMBENET_BR_PORT_NetworkAdapterEnableForwarding</name>
      <anchorfile>network__adapter_8h.html</anchorfile>
      <anchor>abef1eeecd5c7162108de240b03b6168f</anchor>
      <arglist>(EMBENET_BR_PORT_NetworkAdapter *adapter)</arglist>
    </member>
    <member kind="function">
      <type>uint16_t</type>
      <name>EMBENET_BR_PORT_NetworkAdapterSetMTU</name>
      <anchorfile>network__adapter_8h.html</anchorfile>
      <anchor>a22e70754959e0b232ad582058567362b</anchor>
      <arglist>(EMBENET_BR_PORT_NetworkAdapter *adapter, uint16_t mtu)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>EMBENET_BR_PORT_NetworkAdapterWrite</name>
      <anchorfile>network__adapter_8h.html</anchorfile>
      <anchor>afc030ea788e1843613e8cba3c6af8fc2</anchor>
      <arglist>(EMBENET_BR_PORT_NetworkAdapter *adapter, void const *data, size_t data_length)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>random.h</name>
    <path>embenet_port/</path>
    <filename>random_8h.html</filename>
    <member kind="function">
      <type>uint32_t</type>
      <name>EMBENET_BR_PORT_GetRandom</name>
      <anchorfile>random_8h.html</anchorfile>
      <anchor>a903addf9c2bd7b23809d1e5b7300588e</anchor>
      <arglist>(void)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>root_node_connector.h</name>
    <path>embenet_port/</path>
    <filename>root__node__connector_8h.html</filename>
    <member kind="typedef">
      <type>struct EMBENET_BR_PORT_RootNodeConnector</type>
      <name>EMBENET_BR_PORT_RootNodeConnector</name>
      <anchorfile>root__node__connector_8h.html</anchorfile>
      <anchor>a0c541aedfe6e39a9167140556ff45e8f</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>EMBENET_BR_PORT_RootNodeConnector *</type>
      <name>EMBENET_BR_PORT_RootNodeConnectorCreate</name>
      <anchorfile>root__node__connector_8h.html</anchorfile>
      <anchor>a8ad1fae70428998d03ebc373b317f9b6</anchor>
      <arglist>(char const *key)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>EMBENET_BR_PORT_RootNodeConnectorDestroy</name>
      <anchorfile>root__node__connector_8h.html</anchorfile>
      <anchor>a748fd3f8d11f69bc96ddb022b71fa52c</anchor>
      <arglist>(EMBENET_BR_PORT_RootNodeConnector *connector)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>EMBENET_BR_PORT_RootNodeConnectorSetDataReceivedCallback</name>
      <anchorfile>root__node__connector_8h.html</anchorfile>
      <anchor>a2a381e099b000b159f664e46a39d77a6</anchor>
      <arglist>(EMBENET_BR_PORT_RootNodeConnector *connector, void(*callback)(void *context, void const *data, size_t data_length), void *context)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>EMBENET_BR_PORT_RootNodeConnectorSetLinkStateChangedCallback</name>
      <anchorfile>root__node__connector_8h.html</anchorfile>
      <anchor>a165482565766be016c7597214428eb8b</anchor>
      <arglist>(EMBENET_BR_PORT_RootNodeConnector *connector, void(*callback)(void *context, bool is_up), void *context)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>EMBENET_BR_PORT_RootNodeConnectorUp</name>
      <anchorfile>root__node__connector_8h.html</anchorfile>
      <anchor>aa83df79c95069e202b7a17c68f9e1a49</anchor>
      <arglist>(EMBENET_BR_PORT_RootNodeConnector *connector)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>EMBENET_BR_PORT_RootNodeConnectorDown</name>
      <anchorfile>root__node__connector_8h.html</anchorfile>
      <anchor>afc7751e8bb1bde0f993a0936b8aa068f</anchor>
      <arglist>(EMBENET_BR_PORT_RootNodeConnector *connector)</arglist>
    </member>
    <member kind="function">
      <type>char const *</type>
      <name>EMBENET_BR_PORT_RootNodeConnectorGetName</name>
      <anchorfile>root__node__connector_8h.html</anchorfile>
      <anchor>a6be2fa6e27ce4b477b3fd118d6ea9693</anchor>
      <arglist>(EMBENET_BR_PORT_RootNodeConnector const *connector)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>EMBENET_BR_PORT_RootNodeConnectorWrite</name>
      <anchorfile>root__node__connector_8h.html</anchorfile>
      <anchor>a763f6c03411c2fb1dd72f66b7fbb2e0d</anchor>
      <arglist>(EMBENET_BR_PORT_RootNodeConnector *connector, void const *data, size_t data_length)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>embenet_brc_normative_references.md</name>
    <path></path>
    <filename>embenet__brc__normative__references_8md.html</filename>
  </compound>
  <compound kind="file">
    <name>embenet_brc_porting_guide.md</name>
    <path></path>
    <filename>embenet__brc__porting__guide_8md.html</filename>
  </compound>
  <compound kind="file">
    <name>embenet_brc_user_guide.md</name>
    <path></path>
    <filename>embenet__brc__user__guide_8md.html</filename>
  </compound>
  <compound kind="file">
    <name>README.md</name>
    <path></path>
    <filename>_r_e_a_d_m_e_8md.html</filename>
  </compound>
  <compound kind="struct">
    <name>embenet::border_router::events::AsnUpdate</name>
    <filename>structembenet_1_1border__router_1_1events_1_1_asn_update.html</filename>
    <member kind="function">
      <type></type>
      <name>explicit</name>
      <anchorfile>structembenet_1_1border__router_1_1events_1_1_asn_update.html</anchorfile>
      <anchor>a36aeaf86a1b7dccd3e0a696e6fb0fb48</anchor>
      <arglist>(false) const expr AsnUpdate(std</arglist>
    </member>
    <member kind="variable">
      <type>EventType</type>
      <name>type</name>
      <anchorfile>structembenet_1_1border__router_1_1events_1_1_asn_update.html</anchorfile>
      <anchor>a6d972119c94ca8c74081cc46267ea4e0</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>std::uint64_t</type>
      <name>asn</name>
      <anchorfile>structembenet_1_1border__router_1_1events_1_1_asn_update.html</anchorfile>
      <anchor>ab4890183c6af12ae78289b10ce2f871a</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>embenet::border_router::events::AuthenticationAttempt</name>
    <filename>structembenet_1_1border__router_1_1events_1_1_authentication_attempt.html</filename>
    <member kind="function">
      <type>constexpr</type>
      <name>AuthenticationAttempt</name>
      <anchorfile>structembenet_1_1border__router_1_1events_1_1_authentication_attempt.html</anchorfile>
      <anchor>aa8f043759a9f257bf4e1c34568ddbd7e</anchor>
      <arglist>(EUI64 node_eui, AuthResult result)</arglist>
    </member>
    <member kind="variable">
      <type>EventType</type>
      <name>type</name>
      <anchorfile>structembenet_1_1border__router_1_1events_1_1_authentication_attempt.html</anchorfile>
      <anchor>a06b852ea25efea49a9442da1f8cf4aea</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>EUI64</type>
      <name>node_eui</name>
      <anchorfile>structembenet_1_1border__router_1_1events_1_1_authentication_attempt.html</anchorfile>
      <anchor>a5db31f8e585ad873e33aeeff4e4b5c1f</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>AuthResult</type>
      <name>result</name>
      <anchorfile>structembenet_1_1border__router_1_1events_1_1_authentication_attempt.html</anchorfile>
      <anchor>a839d2cbb334f539bba6c7aa7b5221ce5</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>embenet::border_router::BorderRouter</name>
    <filename>classembenet_1_1border__router_1_1_border_router.html</filename>
    <member kind="typedef">
      <type>void(*)(void *context, Event const &amp;event)</type>
      <name>EventHandler</name>
      <anchorfile>classembenet_1_1border__router_1_1_border_router.html</anchorfile>
      <anchor>a44e539d533d616f0bcacfe4a8b53843b</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>std::expected&lt; NetworkPrefix, std::error_code &gt;</type>
      <name>get_network_prefix</name>
      <anchorfile>classembenet_1_1border__router_1_1_border_router.html</anchorfile>
      <anchor>a480116457b4b414603431f1a49aadfb0</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>std::expected&lt; Address, std::error_code &gt;</type>
      <name>get_own_address</name>
      <anchorfile>classembenet_1_1border__router_1_1_border_router.html</anchorfile>
      <anchor>a6eb5b2c90e95b9c3b104ce240316ea76</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>std::expected&lt; Address, std::error_code &gt;</type>
      <name>get_root_node_address</name>
      <anchorfile>classembenet_1_1border__router_1_1_border_router.html</anchorfile>
      <anchor>a798eb92c1925309fa0b32b6c487a1561</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>std::expected&lt; Address, std::error_code &gt;</type>
      <name>get_wan_address</name>
      <anchorfile>classembenet_1_1border__router_1_1_border_router.html</anchorfile>
      <anchor>a034df814821fbf15f7fae0ed20219695</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>std::size_t</type>
      <name>get_nodes_capacity</name>
      <anchorfile>classembenet_1_1border__router_1_1_border_router.html</anchorfile>
      <anchor>a34df7c031fab5aec74f3b4b25f08f051</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>std::size_t</type>
      <name>get_nodes_count</name>
      <anchorfile>classembenet_1_1border__router_1_1_border_router.html</anchorfile>
      <anchor>a75f64c91b4b7d6a0cc47b02bea55fdc8</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>std::span&lt; EUI64 &gt;</type>
      <name>get_nodes</name>
      <anchorfile>classembenet_1_1border__router_1_1_border_router.html</anchorfile>
      <anchor>a74dc767de5cbb837f91c7ec948a86b9d</anchor>
      <arglist>(std::span&lt; EUI64 &gt; buffer, std::size_t offset=0, std::size_t count=std::dynamic_extent) const</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>is_node_present</name>
      <anchorfile>classembenet_1_1border__router_1_1_border_router.html</anchorfile>
      <anchor>a56ca9c9c8e51f86bf263a8eafb3e6ca4</anchor>
      <arglist>(EUI64 uid) const</arglist>
    </member>
    <member kind="function">
      <type>std::optional&lt; NodeDetails &gt;</type>
      <name>get_node_details</name>
      <anchorfile>classembenet_1_1border__router_1_1_border_router.html</anchorfile>
      <anchor>a4db21d689fb1874e7e9d6cdbaedac083</anchor>
      <arglist>(EUI64 uid) const</arglist>
    </member>
    <member kind="function">
      <type>std::chrono::seconds</type>
      <name>get_nodes_inactivity_timeout</name>
      <anchorfile>classembenet_1_1border__router_1_1_border_router.html</anchorfile>
      <anchor>aa0b9a2f40643d747cbe828aee6275747</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>std::error_code</type>
      <name>set_nodes_inactivity_timeout</name>
      <anchorfile>classembenet_1_1border__router_1_1_border_router.html</anchorfile>
      <anchor>a77e29d507fe195635feab45c57860c8e</anchor>
      <arglist>(std::chrono::seconds timeout)</arglist>
    </member>
    <member kind="function">
      <type>std::size_t</type>
      <name>get_multicast_group_capacity</name>
      <anchorfile>classembenet_1_1border__router_1_1_border_router.html</anchorfile>
      <anchor>a4002193a6f17475dbd044feb3e578267</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>std::size_t</type>
      <name>get_multicast_group_count</name>
      <anchorfile>classembenet_1_1border__router_1_1_border_router.html</anchorfile>
      <anchor>a5a6d57fa8ba7a9d34e834218cd169d2e</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>std::span&lt; GID &gt;</type>
      <name>get_multicast_groups</name>
      <anchorfile>classembenet_1_1border__router_1_1_border_router.html</anchorfile>
      <anchor>a7b49c638ea3d65161dd00a7dadbf79b6</anchor>
      <arglist>(std::span&lt; GID &gt; buffer, std::size_t offset=0, std::size_t count=std::dynamic_extent) const</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>is_multicast_group_registered</name>
      <anchorfile>classembenet_1_1border__router_1_1_border_router.html</anchorfile>
      <anchor>ab3d3328e866721035295d44db2ce9c04</anchor>
      <arglist>(GID gid) const</arglist>
    </member>
    <member kind="function">
      <type>std::error_code</type>
      <name>send</name>
      <anchorfile>classembenet_1_1border__router_1_1_border_router.html</anchorfile>
      <anchor>a299785979228df98b61a35b9ba37bcc7</anchor>
      <arglist>(Socket const &amp;socket, Address const &amp;dst, uint16_t dst_port, std::span&lt; std::byte const &gt; data)</arglist>
    </member>
    <member kind="function">
      <type>std::error_code</type>
      <name>register_socket</name>
      <anchorfile>classembenet_1_1border__router_1_1_border_router.html</anchorfile>
      <anchor>ab9ebc88c210654dcba59d27f6d65e975</anchor>
      <arglist>(Socket &amp;socket)</arglist>
    </member>
    <member kind="function">
      <type>std::error_code</type>
      <name>unregister_socket</name>
      <anchorfile>classembenet_1_1border__router_1_1_border_router.html</anchorfile>
      <anchor>ad2ac0ee21e0bfd39ea9897b1f2c7c0cc</anchor>
      <arglist>(Socket &amp;socket)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>is_socket_registered</name>
      <anchorfile>classembenet_1_1border__router_1_1_border_router.html</anchorfile>
      <anchor>a63c2c8181105e248ea4c5f8519e738f0</anchor>
      <arglist>(Socket const &amp;socket) const</arglist>
    </member>
    <member kind="function">
      <type>std::size_t</type>
      <name>get_max_datagram_length</name>
      <anchorfile>classembenet_1_1border__router_1_1_border_router.html</anchorfile>
      <anchor>af2eb195a859645eedbc367a76d006f40</anchor>
      <arglist>(Socket const &amp;socket) const</arglist>
    </member>
    <member kind="function">
      <type>std::size_t</type>
      <name>get_join_rules_capacity</name>
      <anchorfile>classembenet_1_1border__router_1_1_border_router.html</anchorfile>
      <anchor>a3edf655749d43b342233e1cae87e686e</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>std::size_t</type>
      <name>get_join_rules_count</name>
      <anchorfile>classembenet_1_1border__router_1_1_border_router.html</anchorfile>
      <anchor>a4d4f90e8a1a8b6b4b01caef2eeb2ee68</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>std::span&lt; JoinRule &gt;</type>
      <name>get_join_rules</name>
      <anchorfile>classembenet_1_1border__router_1_1_border_router.html</anchorfile>
      <anchor>af369f048b16587293822ee1494e620d4</anchor>
      <arglist>(std::span&lt; JoinRule &gt; buffer, std::size_t offset=0, std::size_t count=std::dynamic_extent) const</arglist>
    </member>
    <member kind="function">
      <type>std::error_code</type>
      <name>add_join_rule</name>
      <anchorfile>classembenet_1_1border__router_1_1_border_router.html</anchorfile>
      <anchor>a2ccc0e1a0c86b1a1f0043909a5ab9fa7</anchor>
      <arglist>(JoinRule const &amp;rule)</arglist>
    </member>
    <member kind="function">
      <type>std::error_code</type>
      <name>remove_join_rule</name>
      <anchorfile>classembenet_1_1border__router_1_1_border_router.html</anchorfile>
      <anchor>adb7a52cf13e91a8ab703774a656a6cdf</anchor>
      <arglist>(JoinRule const &amp;rule)</arglist>
    </member>
    <member kind="function">
      <type>std::error_code</type>
      <name>clear_join_rules</name>
      <anchorfile>classembenet_1_1border__router_1_1_border_router.html</anchorfile>
      <anchor>a752e4e12ca4846f8bf3a01a2e2fa9167</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>std::optional&lt; JoinPolicy &gt;</type>
      <name>get_join_policy</name>
      <anchorfile>classembenet_1_1border__router_1_1_border_router.html</anchorfile>
      <anchor>a9e785b8d4704421ae520c0896bdea979</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>std::error_code</type>
      <name>set_join_policy</name>
      <anchorfile>classembenet_1_1border__router_1_1_border_router.html</anchorfile>
      <anchor>a6b7a00425858a6e248428425af148490</anchor>
      <arglist>(std::optional&lt; JoinPolicy &gt; policy)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>configure_promiscuous_mode</name>
      <anchorfile>classembenet_1_1border__router_1_1_border_router.html</anchorfile>
      <anchor>a9b9c8bb2b5bad1066ffa4eaa3f29c21b</anchor>
      <arglist>(bool enable=true)</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static Capabilities</type>
      <name>get_max_capabilities</name>
      <anchorfile>classembenet_1_1border__router_1_1_border_router.html</anchorfile>
      <anchor>a14b708be0a61c8e2e7441f0cb8da5b68</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static Capabilities</type>
      <name>get_default_capabilities</name>
      <anchorfile>classembenet_1_1border__router_1_1_border_router.html</anchorfile>
      <anchor>acac7a697969d7140db6c0cec770b5fad</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static std::expected&lt; BorderRouter, std::error_code &gt;</type>
      <name>create</name>
      <anchorfile>classembenet_1_1border__router_1_1_border_router.html</anchorfile>
      <anchor>ad8df4ff48245b8acfbd997753d76ab9f</anchor>
      <arglist>(Capabilities capabilities=get_default_capabilities())</arglist>
    </member>
    <member kind="function">
      <type>std::error_code</type>
      <name>subscribe_to_events</name>
      <anchorfile>classembenet_1_1border__router_1_1_border_router.html</anchorfile>
      <anchor>a557c2181ab0dd309b0bbaf527559aa92</anchor>
      <arglist>(EventHandler handler, void *context)</arglist>
    </member>
    <member kind="function">
      <type>std::error_code</type>
      <name>configure</name>
      <anchorfile>classembenet_1_1border__router_1_1_border_router.html</anchorfile>
      <anchor>a8d2346e44c4464ac76fd3de9ba936dab</anchor>
      <arglist>(std::string_view root_node_interface, NetworkParameters const &amp;config, std::string_view wan_interface={})</arglist>
    </member>
    <member kind="function">
      <type>std::error_code</type>
      <name>up</name>
      <anchorfile>classembenet_1_1border__router_1_1_border_router.html</anchorfile>
      <anchor>a9b39935f648a23fdde6252de6cfdd008</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>is_up</name>
      <anchorfile>classembenet_1_1border__router_1_1_border_router.html</anchorfile>
      <anchor>aba182db0f5145a67bce23ddb119ec50d</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>std::error_code</type>
      <name>down</name>
      <anchorfile>classembenet_1_1border__router_1_1_border_router.html</anchorfile>
      <anchor>aedc5174fa959626dafb8f598ef10eae6</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>std::error_code</type>
      <name>process</name>
      <anchorfile>classembenet_1_1border__router_1_1_border_router.html</anchorfile>
      <anchor>ad602d6e86085de20dbf129797f737d11</anchor>
      <arglist>(std::chrono::milliseconds timeout)</arglist>
    </member>
    <member kind="function">
      <type>std::expected&lt; NetworkPrefix, std::error_code &gt;</type>
      <name>get_network_prefix</name>
      <anchorfile>classembenet_1_1border__router_1_1_border_router.html</anchorfile>
      <anchor>a480116457b4b414603431f1a49aadfb0</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>std::expected&lt; Address, std::error_code &gt;</type>
      <name>get_own_address</name>
      <anchorfile>classembenet_1_1border__router_1_1_border_router.html</anchorfile>
      <anchor>a6eb5b2c90e95b9c3b104ce240316ea76</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>std::expected&lt; Address, std::error_code &gt;</type>
      <name>get_root_node_address</name>
      <anchorfile>classembenet_1_1border__router_1_1_border_router.html</anchorfile>
      <anchor>a798eb92c1925309fa0b32b6c487a1561</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>std::expected&lt; Address, std::error_code &gt;</type>
      <name>get_wan_address</name>
      <anchorfile>classembenet_1_1border__router_1_1_border_router.html</anchorfile>
      <anchor>a034df814821fbf15f7fae0ed20219695</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>std::size_t</type>
      <name>get_nodes_capacity</name>
      <anchorfile>classembenet_1_1border__router_1_1_border_router.html</anchorfile>
      <anchor>a34df7c031fab5aec74f3b4b25f08f051</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>std::size_t</type>
      <name>get_nodes_count</name>
      <anchorfile>classembenet_1_1border__router_1_1_border_router.html</anchorfile>
      <anchor>a75f64c91b4b7d6a0cc47b02bea55fdc8</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>std::span&lt; EUI64 &gt;</type>
      <name>get_nodes</name>
      <anchorfile>classembenet_1_1border__router_1_1_border_router.html</anchorfile>
      <anchor>a74dc767de5cbb837f91c7ec948a86b9d</anchor>
      <arglist>(std::span&lt; EUI64 &gt; buffer, std::size_t offset=0, std::size_t count=std::dynamic_extent) const</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>is_node_present</name>
      <anchorfile>classembenet_1_1border__router_1_1_border_router.html</anchorfile>
      <anchor>a56ca9c9c8e51f86bf263a8eafb3e6ca4</anchor>
      <arglist>(EUI64 uid) const</arglist>
    </member>
    <member kind="function">
      <type>std::optional&lt; NodeDetails &gt;</type>
      <name>get_node_details</name>
      <anchorfile>classembenet_1_1border__router_1_1_border_router.html</anchorfile>
      <anchor>a4db21d689fb1874e7e9d6cdbaedac083</anchor>
      <arglist>(EUI64 uid) const</arglist>
    </member>
    <member kind="function">
      <type>std::chrono::seconds</type>
      <name>get_nodes_inactivity_timeout</name>
      <anchorfile>classembenet_1_1border__router_1_1_border_router.html</anchorfile>
      <anchor>aa0b9a2f40643d747cbe828aee6275747</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>std::error_code</type>
      <name>set_nodes_inactivity_timeout</name>
      <anchorfile>classembenet_1_1border__router_1_1_border_router.html</anchorfile>
      <anchor>a77e29d507fe195635feab45c57860c8e</anchor>
      <arglist>(std::chrono::seconds timeout)</arglist>
    </member>
    <member kind="function">
      <type>std::size_t</type>
      <name>get_multicast_group_capacity</name>
      <anchorfile>classembenet_1_1border__router_1_1_border_router.html</anchorfile>
      <anchor>a4002193a6f17475dbd044feb3e578267</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>std::size_t</type>
      <name>get_multicast_group_count</name>
      <anchorfile>classembenet_1_1border__router_1_1_border_router.html</anchorfile>
      <anchor>a5a6d57fa8ba7a9d34e834218cd169d2e</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>std::span&lt; GID &gt;</type>
      <name>get_multicast_groups</name>
      <anchorfile>classembenet_1_1border__router_1_1_border_router.html</anchorfile>
      <anchor>a7b49c638ea3d65161dd00a7dadbf79b6</anchor>
      <arglist>(std::span&lt; GID &gt; buffer, std::size_t offset=0, std::size_t count=std::dynamic_extent) const</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>is_multicast_group_registered</name>
      <anchorfile>classembenet_1_1border__router_1_1_border_router.html</anchorfile>
      <anchor>ab3d3328e866721035295d44db2ce9c04</anchor>
      <arglist>(GID gid) const</arglist>
    </member>
    <member kind="function">
      <type>std::error_code</type>
      <name>send</name>
      <anchorfile>classembenet_1_1border__router_1_1_border_router.html</anchorfile>
      <anchor>a299785979228df98b61a35b9ba37bcc7</anchor>
      <arglist>(Socket const &amp;socket, Address const &amp;dst, uint16_t dst_port, std::span&lt; std::byte const &gt; data)</arglist>
    </member>
    <member kind="function">
      <type>std::error_code</type>
      <name>register_socket</name>
      <anchorfile>classembenet_1_1border__router_1_1_border_router.html</anchorfile>
      <anchor>ab9ebc88c210654dcba59d27f6d65e975</anchor>
      <arglist>(Socket &amp;socket)</arglist>
    </member>
    <member kind="function">
      <type>std::error_code</type>
      <name>unregister_socket</name>
      <anchorfile>classembenet_1_1border__router_1_1_border_router.html</anchorfile>
      <anchor>ad2ac0ee21e0bfd39ea9897b1f2c7c0cc</anchor>
      <arglist>(Socket &amp;socket)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>is_socket_registered</name>
      <anchorfile>classembenet_1_1border__router_1_1_border_router.html</anchorfile>
      <anchor>a63c2c8181105e248ea4c5f8519e738f0</anchor>
      <arglist>(Socket const &amp;socket) const</arglist>
    </member>
    <member kind="function">
      <type>std::size_t</type>
      <name>get_max_datagram_length</name>
      <anchorfile>classembenet_1_1border__router_1_1_border_router.html</anchorfile>
      <anchor>af2eb195a859645eedbc367a76d006f40</anchor>
      <arglist>(Socket const &amp;socket) const</arglist>
    </member>
    <member kind="function">
      <type>std::size_t</type>
      <name>get_join_rules_capacity</name>
      <anchorfile>classembenet_1_1border__router_1_1_border_router.html</anchorfile>
      <anchor>a3edf655749d43b342233e1cae87e686e</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>std::size_t</type>
      <name>get_join_rules_count</name>
      <anchorfile>classembenet_1_1border__router_1_1_border_router.html</anchorfile>
      <anchor>a4d4f90e8a1a8b6b4b01caef2eeb2ee68</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>std::span&lt; JoinRule &gt;</type>
      <name>get_join_rules</name>
      <anchorfile>classembenet_1_1border__router_1_1_border_router.html</anchorfile>
      <anchor>af369f048b16587293822ee1494e620d4</anchor>
      <arglist>(std::span&lt; JoinRule &gt; buffer, std::size_t offset=0, std::size_t count=std::dynamic_extent) const</arglist>
    </member>
    <member kind="function">
      <type>std::error_code</type>
      <name>add_join_rule</name>
      <anchorfile>classembenet_1_1border__router_1_1_border_router.html</anchorfile>
      <anchor>a2ccc0e1a0c86b1a1f0043909a5ab9fa7</anchor>
      <arglist>(JoinRule const &amp;rule)</arglist>
    </member>
    <member kind="function">
      <type>std::error_code</type>
      <name>remove_join_rule</name>
      <anchorfile>classembenet_1_1border__router_1_1_border_router.html</anchorfile>
      <anchor>adb7a52cf13e91a8ab703774a656a6cdf</anchor>
      <arglist>(JoinRule const &amp;rule)</arglist>
    </member>
    <member kind="function">
      <type>std::error_code</type>
      <name>clear_join_rules</name>
      <anchorfile>classembenet_1_1border__router_1_1_border_router.html</anchorfile>
      <anchor>a752e4e12ca4846f8bf3a01a2e2fa9167</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>std::optional&lt; JoinPolicy &gt;</type>
      <name>get_join_policy</name>
      <anchorfile>classembenet_1_1border__router_1_1_border_router.html</anchorfile>
      <anchor>a9e785b8d4704421ae520c0896bdea979</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>std::error_code</type>
      <name>set_join_policy</name>
      <anchorfile>classembenet_1_1border__router_1_1_border_router.html</anchorfile>
      <anchor>a6b7a00425858a6e248428425af148490</anchor>
      <arglist>(std::optional&lt; JoinPolicy &gt; policy)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>configure_promiscuous_mode</name>
      <anchorfile>classembenet_1_1border__router_1_1_border_router.html</anchorfile>
      <anchor>a9b9c8bb2b5bad1066ffa4eaa3f29c21b</anchor>
      <arglist>(bool enable=true)</arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>embenet::border_router::events::DataOnUnregisteredPort</name>
    <filename>structembenet_1_1border__router_1_1events_1_1_data_on_unregistered_port.html</filename>
    <member kind="function">
      <type>constexpr</type>
      <name>DataOnUnregisteredPort</name>
      <anchorfile>structembenet_1_1border__router_1_1events_1_1_data_on_unregistered_port.html</anchorfile>
      <anchor>a37abba63c8f7af3bc6c5cdf5fc18a6e9</anchor>
      <arglist>(std::uint16_t destination_port, std::uint16_t source_port)</arglist>
    </member>
    <member kind="variable">
      <type>EventType</type>
      <name>type</name>
      <anchorfile>structembenet_1_1border__router_1_1events_1_1_data_on_unregistered_port.html</anchorfile>
      <anchor>abd5adb729d7e6f737512991566126259</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>std::uint16_t</type>
      <name>destination_port</name>
      <anchorfile>structembenet_1_1border__router_1_1events_1_1_data_on_unregistered_port.html</anchorfile>
      <anchor>a50d6b52e8c73cfcf70c367b4c3a273ac</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>std::uint16_t</type>
      <name>source_port</name>
      <anchorfile>structembenet_1_1border__router_1_1events_1_1_data_on_unregistered_port.html</anchorfile>
      <anchor>a8cec6717d0cd5d700ff28c928a0ba5c5</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>EMBENET_BR_Capabilities</name>
    <filename>struct_e_m_b_e_n_e_t___b_r___capabilities.html</filename>
    <member kind="variable">
      <type>uint32_t</type>
      <name>max_nodes</name>
      <anchorfile>struct_e_m_b_e_n_e_t___b_r___capabilities.html</anchorfile>
      <anchor>a8ff9c64c9543a4443b966e6a8d8a7a5d</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint32_t</type>
      <name>max_groups</name>
      <anchorfile>struct_e_m_b_e_n_e_t___b_r___capabilities.html</anchorfile>
      <anchor>a8f07bf898a0b59a455de22df26680c99</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint32_t</type>
      <name>max_join_rules</name>
      <anchorfile>struct_e_m_b_e_n_e_t___b_r___capabilities.html</anchorfile>
      <anchor>ac0740e97ce97deb6d4c70f9b8741811b</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint32_t</type>
      <name>packet_queue_capacity</name>
      <anchorfile>struct_e_m_b_e_n_e_t___b_r___capabilities.html</anchorfile>
      <anchor>a220ea51e649241b31e2c7afa71a96dea</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint32_t</type>
      <name>event_queue_capacity</name>
      <anchorfile>struct_e_m_b_e_n_e_t___b_r___capabilities.html</anchorfile>
      <anchor>af8035385b9f9888cf7b5c28eaee890a1</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>EMBENET_BR_CryptoKey</name>
    <filename>struct_e_m_b_e_n_e_t___b_r___crypto_key.html</filename>
    <member kind="variable">
      <type>uint8_t</type>
      <name>value</name>
      <anchorfile>struct_e_m_b_e_n_e_t___b_r___crypto_key.html</anchorfile>
      <anchor>ac28428b17c7d980bf42dac56f1ad833a</anchor>
      <arglist>[EMBENET_BR_CRYPTO_KEY_LENGTH]</arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>EMBENET_BR_EventHandlers</name>
    <filename>struct_e_m_b_e_n_e_t___b_r___event_handlers.html</filename>
    <member kind="variable">
      <type>EMBENET_BR_OnNetworkStarted</type>
      <name>on_network_started</name>
      <anchorfile>struct_e_m_b_e_n_e_t___b_r___event_handlers.html</anchorfile>
      <anchor>aebec68c7a76e01fe5ac8013d74301c23</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>EMBENET_BR_OnNetworkStopped</type>
      <name>on_network_stopped</name>
      <anchorfile>struct_e_m_b_e_n_e_t___b_r___event_handlers.html</anchorfile>
      <anchor>a64993b8d5994d4a0eb27d551d692c7ee</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>EMBENET_BR_OnNodeJoined</type>
      <name>on_node_joined</name>
      <anchorfile>struct_e_m_b_e_n_e_t___b_r___event_handlers.html</anchorfile>
      <anchor>a1c1ad1b066fc4e621e367685fd54b7e2</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>EMBENET_BR_OnNodeLeft</type>
      <name>on_node_left</name>
      <anchorfile>struct_e_m_b_e_n_e_t___b_r___event_handlers.html</anchorfile>
      <anchor>a4f8b96ed2dc2cf4ae43b605612c841e6</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>EMBENET_BR_OnAuthenticationAttempt</type>
      <name>on_authentication_attempt</name>
      <anchorfile>struct_e_m_b_e_n_e_t___b_r___event_handlers.html</anchorfile>
      <anchor>a014191c519cde876e563382889964c16</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>EMBENET_BR_OnNodeAddedToGroup</type>
      <name>on_node_added_to_group</name>
      <anchorfile>struct_e_m_b_e_n_e_t___b_r___event_handlers.html</anchorfile>
      <anchor>a6302f2d957efa57220cc282e4f3cac7b</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>EMBENET_BR_OnNodeRemovedFromGroup</type>
      <name>on_node_removed_from_group</name>
      <anchorfile>struct_e_m_b_e_n_e_t___b_r___event_handlers.html</anchorfile>
      <anchor>a4d2d272303eddbf403bb6d6bea0a01e4</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>EMBENET_BR_OnDataOnUnregisteredPort</type>
      <name>on_datagram_on_unregistered_port</name>
      <anchorfile>struct_e_m_b_e_n_e_t___b_r___event_handlers.html</anchorfile>
      <anchor>a254aa69aeb49631c6836bddd63649ad0</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>EMBENET_BR_OnPacketDropped</type>
      <name>on_packet_dropped</name>
      <anchorfile>struct_e_m_b_e_n_e_t___b_r___event_handlers.html</anchorfile>
      <anchor>a69de7bf3cc8ccb11c43d3c82895b5b58</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>EMBENET_BR_OnAsnUpdate</type>
      <name>on_asn_update</name>
      <anchorfile>struct_e_m_b_e_n_e_t___b_r___event_handlers.html</anchorfile>
      <anchor>a8bf8dac8fc759dc017d54e55f2463938</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>EMBENET_BR_OnRootNodeConnectionFailed</type>
      <name>on_root_node_connection_failed</name>
      <anchorfile>struct_e_m_b_e_n_e_t___b_r___event_handlers.html</anchorfile>
      <anchor>a312c1bdad5c7f84804085de042879373</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>EMBENET_BR_OnRootNodeIdentified</type>
      <name>on_root_node_identified</name>
      <anchorfile>struct_e_m_b_e_n_e_t___b_r___event_handlers.html</anchorfile>
      <anchor>ad2af48a3bdcffd7a84034b120910182b</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>EMBENET_BR_GroupsList</name>
    <filename>struct_e_m_b_e_n_e_t___b_r___groups_list.html</filename>
    <member kind="variable">
      <type>size_t</type>
      <name>total_count</name>
      <anchorfile>struct_e_m_b_e_n_e_t___b_r___groups_list.html</anchorfile>
      <anchor>af554f9a7b139f792a99676ad0f4a97f5</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>size_t</type>
      <name>count</name>
      <anchorfile>struct_e_m_b_e_n_e_t___b_r___groups_list.html</anchorfile>
      <anchor>a2d3e9e4efea7ab0694e265b0ffee7516</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>EMBENET_GroupId *</type>
      <name>groups</name>
      <anchorfile>struct_e_m_b_e_n_e_t___b_r___groups_list.html</anchorfile>
      <anchor>a5b6c52b9a0f75d2e309426af49a5ee59</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>EMBENET_BR_JoinPolicy</name>
    <filename>struct_e_m_b_e_n_e_t___b_r___join_policy.html</filename>
    <member kind="variable">
      <type>int32_t</type>
      <name>max_immediate_requests</name>
      <anchorfile>struct_e_m_b_e_n_e_t___b_r___join_policy.html</anchorfile>
      <anchor>ab3be6d0e38903e14f06b808812d8ac08</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int32_t</type>
      <name>min_request_interval_ms</name>
      <anchorfile>struct_e_m_b_e_n_e_t___b_r___join_policy.html</anchorfile>
      <anchor>a18d80879bb656437ef51fa317837348e</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>EMBENET_BR_JoinRule</name>
    <filename>struct_e_m_b_e_n_e_t___b_r___join_rule.html</filename>
    <member kind="variable">
      <type>EMBENET_EUI64</type>
      <name>uid</name>
      <anchorfile>struct_e_m_b_e_n_e_t___b_r___join_rule.html</anchorfile>
      <anchor>a9d16e873315d6d4bc71c93047d44568d</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>EMBENET_BR_CryptoKey</type>
      <name>psk</name>
      <anchorfile>struct_e_m_b_e_n_e_t___b_r___join_rule.html</anchorfile>
      <anchor>a612802e5fdc80f80a6007bb64b6e5306</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>EMBENET_BR_JoinRulesList</name>
    <filename>struct_e_m_b_e_n_e_t___b_r___join_rules_list.html</filename>
    <member kind="variable">
      <type>size_t</type>
      <name>total_count</name>
      <anchorfile>struct_e_m_b_e_n_e_t___b_r___join_rules_list.html</anchorfile>
      <anchor>a639cea4b2f8b545b73ea1ca5849f79bc</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>size_t</type>
      <name>count</name>
      <anchorfile>struct_e_m_b_e_n_e_t___b_r___join_rules_list.html</anchorfile>
      <anchor>aa089fda5b25d912ed33b690f51edb9fc</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>EMBENET_BR_JoinRule *</type>
      <name>rules</name>
      <anchorfile>struct_e_m_b_e_n_e_t___b_r___join_rules_list.html</anchorfile>
      <anchor>a958ee3dcc276c07ddfb2e92a0603229b</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>EMBENET_BR_NetworkParameters</name>
    <filename>struct_e_m_b_e_n_e_t___b_r___network_parameters.html</filename>
    <member kind="variable">
      <type>EMBENET_BR_CryptoKey</type>
      <name>k1</name>
      <anchorfile>struct_e_m_b_e_n_e_t___b_r___network_parameters.html</anchorfile>
      <anchor>a2ebc34ae0401bb02bb9c0933fe920b69</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>EMBENET_PAN_Prefix</type>
      <name>pan_prefix</name>
      <anchorfile>struct_e_m_b_e_n_e_t___b_r___network_parameters.html</anchorfile>
      <anchor>a7cfb30a987d6928a117d19b5cea2588b</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>EMBENET_EUI64</type>
      <name>root_node_uid_override</name>
      <anchorfile>struct_e_m_b_e_n_e_t___b_r___network_parameters.html</anchorfile>
      <anchor>ace177ed1c34137f1d0f9fdddef63ec3a</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>EMBENET_EUI64</type>
      <name>own_uid</name>
      <anchorfile>struct_e_m_b_e_n_e_t___b_r___network_parameters.html</anchorfile>
      <anchor>aef461c3645b191ba2ea8448fd8e35b50</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>EMBENET_PANID</type>
      <name>pan_id</name>
      <anchorfile>struct_e_m_b_e_n_e_t___b_r___network_parameters.html</anchorfile>
      <anchor>a81bc63f69b3829dfb0acf5ea97c6febb</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint8_t</type>
      <name>pan_data_size</name>
      <anchorfile>struct_e_m_b_e_n_e_t___b_r___network_parameters.html</anchorfile>
      <anchor>ace645985dfee9c7f6b0ac88e19aa5693</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint8_t</type>
      <name>pan_data</name>
      <anchorfile>struct_e_m_b_e_n_e_t___b_r___network_parameters.html</anchorfile>
      <anchor>ae305a00f9299c8ad676d535ea7648113</anchor>
      <arglist>[EMBENET_BR_MAX_PAN_DATA_SIZE]</arglist>
    </member>
    <member kind="variable">
      <type>uint32_t</type>
      <name>root_node_watchdog_timeout_s</name>
      <anchorfile>struct_e_m_b_e_n_e_t___b_r___network_parameters.html</anchorfile>
      <anchor>a74c27ab93e1a72c01a9ebf7d973891c7</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint32_t</type>
      <name>root_node_response_timeout_ms</name>
      <anchorfile>struct_e_m_b_e_n_e_t___b_r___network_parameters.html</anchorfile>
      <anchor>a56cbc82d78dc2b1288f896b1494e320b</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>EMBENET_BR_NodeDetails</name>
    <filename>struct_e_m_b_e_n_e_t___b_r___node_details.html</filename>
    <member kind="variable">
      <type>EMBENET_EUI64</type>
      <name>uid</name>
      <anchorfile>struct_e_m_b_e_n_e_t___b_r___node_details.html</anchorfile>
      <anchor>aff49c0d53cc807acc2f3a515f45230aa</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>EMBENET_EUI64</type>
      <name>parent_uid</name>
      <anchorfile>struct_e_m_b_e_n_e_t___b_r___node_details.html</anchorfile>
      <anchor>ac2abe8d2d1bc939baee4e43a8f4292ec</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int64_t</type>
      <name>last_seen</name>
      <anchorfile>struct_e_m_b_e_n_e_t___b_r___node_details.html</anchorfile>
      <anchor>af4b2b2350e18ba6ab75b8cf067b5af18</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>size_t</type>
      <name>registered_group_count</name>
      <anchorfile>struct_e_m_b_e_n_e_t___b_r___node_details.html</anchorfile>
      <anchor>ab7c6e8e36b9f0d829c855b1aebb61d79</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>EMBENET_GroupId</type>
      <name>groups</name>
      <anchorfile>struct_e_m_b_e_n_e_t___b_r___node_details.html</anchorfile>
      <anchor>af9e919644fba4040491c6cbe66126d65</anchor>
      <arglist>[EMBENET_BR_MAX_NODE_MULTICAST_GROUPS]</arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>EMBENET_BR_NodesList</name>
    <filename>struct_e_m_b_e_n_e_t___b_r___nodes_list.html</filename>
    <member kind="variable">
      <type>size_t</type>
      <name>total_count</name>
      <anchorfile>struct_e_m_b_e_n_e_t___b_r___nodes_list.html</anchorfile>
      <anchor>ace7ed91a6ee16888090de9b79a78d51c</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>size_t</type>
      <name>count</name>
      <anchorfile>struct_e_m_b_e_n_e_t___b_r___nodes_list.html</anchorfile>
      <anchor>a0f745e10c17d94734f8e34ffbf07129c</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>EMBENET_EUI64 *</type>
      <name>nodes</name>
      <anchorfile>struct_e_m_b_e_n_e_t___b_r___nodes_list.html</anchorfile>
      <anchor>abfd1d8f6b4fa35fe6b3c933d0e8a1e0f</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>embenet::border_router::Event</name>
    <filename>structembenet_1_1border__router_1_1_event.html</filename>
    <member kind="variable">
      <type>EventType</type>
      <name>type</name>
      <anchorfile>structembenet_1_1border__router_1_1_event.html</anchorfile>
      <anchor>aabd2a53f47a31d75b6d957785244086e</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>embenet::border_router::events::NetworkStarted</name>
    <filename>structembenet_1_1border__router_1_1events_1_1_network_started.html</filename>
    <member kind="function">
      <type></type>
      <name>explicit</name>
      <anchorfile>structembenet_1_1border__router_1_1events_1_1_network_started.html</anchorfile>
      <anchor>a022646a5a78f99d83131fde41a114f05</anchor>
      <arglist>(false) const expr NetworkStarted(EUI64 root_uid)</arglist>
    </member>
    <member kind="variable">
      <type>EventType</type>
      <name>type</name>
      <anchorfile>structembenet_1_1border__router_1_1events_1_1_network_started.html</anchorfile>
      <anchor>aaf0ed2ea8cf9c2bf72501914e4701de1</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>EUI64</type>
      <name>root_uid</name>
      <anchorfile>structembenet_1_1border__router_1_1events_1_1_network_started.html</anchorfile>
      <anchor>a4dcf2935d8bc60b8e648cbaba8082432</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>embenet::border_router::events::NetworkStopped</name>
    <filename>structembenet_1_1border__router_1_1events_1_1_network_stopped.html</filename>
    <member kind="variable">
      <type>EventType</type>
      <name>type</name>
      <anchorfile>structembenet_1_1border__router_1_1events_1_1_network_stopped.html</anchorfile>
      <anchor>a96d78ecd2a0be1b0d567a32321bc086d</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>embenet::border_router::events::NodeJoinedGroup</name>
    <filename>structembenet_1_1border__router_1_1events_1_1_node_joined_group.html</filename>
    <member kind="function">
      <type>constexpr</type>
      <name>NodeJoinedGroup</name>
      <anchorfile>structembenet_1_1border__router_1_1events_1_1_node_joined_group.html</anchorfile>
      <anchor>aaa8b58c3ae30437a60648c8fc4f9e82c</anchor>
      <arglist>(EUI64 node_eui, GID group_uid)</arglist>
    </member>
    <member kind="variable">
      <type>EventType</type>
      <name>type</name>
      <anchorfile>structembenet_1_1border__router_1_1events_1_1_node_joined_group.html</anchorfile>
      <anchor>ab40ad3d3149e20be7128018034954eca</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>EUI64</type>
      <name>node_eui</name>
      <anchorfile>structembenet_1_1border__router_1_1events_1_1_node_joined_group.html</anchorfile>
      <anchor>ab2223a6f1e4e5cc6f29423e42f1212ee</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>GID</type>
      <name>group_uid</name>
      <anchorfile>structembenet_1_1border__router_1_1events_1_1_node_joined_group.html</anchorfile>
      <anchor>a91bb40328ea0d9dae2203db4b50d821f</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>embenet::border_router::events::NodeJoinedNetwork</name>
    <filename>structembenet_1_1border__router_1_1events_1_1_node_joined_network.html</filename>
    <member kind="function">
      <type></type>
      <name>explicit</name>
      <anchorfile>structembenet_1_1border__router_1_1events_1_1_node_joined_network.html</anchorfile>
      <anchor>a915b3b8f9b50e3f37bed6a5cd425037f</anchor>
      <arglist>(false) const expr NodeJoinedNetwork(EUI64 node_eui)</arglist>
    </member>
    <member kind="variable">
      <type>EventType</type>
      <name>type</name>
      <anchorfile>structembenet_1_1border__router_1_1events_1_1_node_joined_network.html</anchorfile>
      <anchor>a17b99d0f1cc3195f4a77bf2e11d754b0</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>EUI64</type>
      <name>node_eui</name>
      <anchorfile>structembenet_1_1border__router_1_1events_1_1_node_joined_network.html</anchorfile>
      <anchor>a2167dac0fcb3661784695bfe48f3dcdd</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>embenet::border_router::events::NodeLeftGroup</name>
    <filename>structembenet_1_1border__router_1_1events_1_1_node_left_group.html</filename>
    <member kind="function">
      <type>constexpr</type>
      <name>NodeLeftGroup</name>
      <anchorfile>structembenet_1_1border__router_1_1events_1_1_node_left_group.html</anchorfile>
      <anchor>a8428aee6f65ccca342cbe6dc5bff1dc9</anchor>
      <arglist>(EUI64 node_eui, GID group_uid)</arglist>
    </member>
    <member kind="variable">
      <type>EventType</type>
      <name>type</name>
      <anchorfile>structembenet_1_1border__router_1_1events_1_1_node_left_group.html</anchorfile>
      <anchor>ac83017faf4724c8d0facdbc1ae84eff9</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>EUI64</type>
      <name>node_eui</name>
      <anchorfile>structembenet_1_1border__router_1_1events_1_1_node_left_group.html</anchorfile>
      <anchor>a8e4dab222f1b2a13ed7b45e7de299388</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>embenet::border_router::GID</type>
      <name>group_uid</name>
      <anchorfile>structembenet_1_1border__router_1_1events_1_1_node_left_group.html</anchorfile>
      <anchor>a04983922cf41e88d77b92a0511fb0e12</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>embenet::border_router::events::NodeLeftNetwork</name>
    <filename>structembenet_1_1border__router_1_1events_1_1_node_left_network.html</filename>
    <member kind="function">
      <type></type>
      <name>explicit</name>
      <anchorfile>structembenet_1_1border__router_1_1events_1_1_node_left_network.html</anchorfile>
      <anchor>aa8e957551fe8061d0887a2e90a292c33</anchor>
      <arglist>(false) const expr NodeLeftNetwork(EUI64 node_eui)</arglist>
    </member>
    <member kind="variable">
      <type>EventType</type>
      <name>type</name>
      <anchorfile>structembenet_1_1border__router_1_1events_1_1_node_left_network.html</anchorfile>
      <anchor>acc2f5df5d34f10548751d72fca81aa04</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>EUI64</type>
      <name>node_eui</name>
      <anchorfile>structembenet_1_1border__router_1_1events_1_1_node_left_network.html</anchorfile>
      <anchor>a8173b7008b7af01271f242e91d9384f3</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>embenet::border_router::events::PacketDropped</name>
    <filename>structembenet_1_1border__router_1_1events_1_1_packet_dropped.html</filename>
    <member kind="function">
      <type></type>
      <name>explicit</name>
      <anchorfile>structembenet_1_1border__router_1_1events_1_1_packet_dropped.html</anchorfile>
      <anchor>a5ad1f38977ca80f733e28477c82c70e5</anchor>
      <arglist>(false) const expr PacketDropped(std</arglist>
    </member>
    <member kind="variable">
      <type>EventType</type>
      <name>type</name>
      <anchorfile>structembenet_1_1border__router_1_1events_1_1_packet_dropped.html</anchorfile>
      <anchor>aea393057b2330cfa04ba6bd11d37d40c</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>std::uint8_t</type>
      <name>protocol</name>
      <anchorfile>structembenet_1_1border__router_1_1events_1_1_packet_dropped.html</anchorfile>
      <anchor>a3e613cb205eed222a95e40818519ef21</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>embenet::border_router::events::RootNodeConnectionFailed</name>
    <filename>structembenet_1_1border__router_1_1events_1_1_root_node_connection_failed.html</filename>
    <member kind="variable">
      <type>EventType</type>
      <name>type</name>
      <anchorfile>structembenet_1_1border__router_1_1events_1_1_root_node_connection_failed.html</anchorfile>
      <anchor>a9194f32b74d35c7626b467c338bf16a5</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>embenet::border_router::events::RootNodeIdentified</name>
    <filename>structembenet_1_1border__router_1_1events_1_1_root_node_identified.html</filename>
    <member kind="function">
      <type></type>
      <name>explicit</name>
      <anchorfile>structembenet_1_1border__router_1_1events_1_1_root_node_identified.html</anchorfile>
      <anchor>a8b04d4417e649fcf32397031886e6ade</anchor>
      <arglist>(false) const expr RootNodeIdentified(EUI64 root_uid)</arglist>
    </member>
    <member kind="variable">
      <type>EventType</type>
      <name>type</name>
      <anchorfile>structembenet_1_1border__router_1_1events_1_1_root_node_identified.html</anchorfile>
      <anchor>a888c0aae98e5c3b4d248c288ab72388e</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>EUI64</type>
      <name>root_uid</name>
      <anchorfile>structembenet_1_1border__router_1_1events_1_1_root_node_identified.html</anchorfile>
      <anchor>a08abfe70b1457a2e9a2589730cd97677</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="namespace">
    <name>embenet</name>
    <filename>namespaceembenet.html</filename>
    <namespace>embenet::border_router</namespace>
  </compound>
  <compound kind="namespace">
    <name>embenet::border_router</name>
    <filename>namespaceembenet_1_1border__router.html</filename>
    <namespace>embenet::border_router::events</namespace>
    <class kind="class">embenet::border_router::BorderRouter</class>
    <class kind="struct">embenet::border_router::Event</class>
    <member kind="typedef">
      <type>EMBENET_BR_Capabilities</type>
      <name>Capabilities</name>
      <anchorfile>namespaceembenet_1_1border__router.html</anchorfile>
      <anchor>a106d6741c3537f3a002e9620c47a223b</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>EMBENET_NetworkPrefix</type>
      <name>NetworkPrefix</name>
      <anchorfile>namespaceembenet_1_1border__router.html</anchorfile>
      <anchor>a1f216423c292d8134b1203d98aaadb7d</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>EMBENET_EUI64</type>
      <name>EUI64</name>
      <anchorfile>namespaceembenet_1_1border__router.html</anchorfile>
      <anchor>a9e6f6aa0e4905da1e2bf5b631e5aa558</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>EMBENET_IPV6</type>
      <name>Address</name>
      <anchorfile>namespaceembenet_1_1border__router.html</anchorfile>
      <anchor>aef56c07e3c97056ecc65348d24985f71</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>EMBENET_GroupId</type>
      <name>GID</name>
      <anchorfile>namespaceembenet_1_1border__router.html</anchorfile>
      <anchor>a6ba75e43d86919cc01187e46f69fd2dc</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>EMBENET_UDP_SocketDescriptor</type>
      <name>Socket</name>
      <anchorfile>namespaceembenet_1_1border__router.html</anchorfile>
      <anchor>ab92c416e7f9cf04ec9dd7fd6c45b25e3</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>EMBENET_BR_JoinRule</type>
      <name>JoinRule</name>
      <anchorfile>namespaceembenet_1_1border__router.html</anchorfile>
      <anchor>a0319dc6436ba80d736e20dbed77b2b51</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>EMBENET_BR_JoinPolicy</type>
      <name>JoinPolicy</name>
      <anchorfile>namespaceembenet_1_1border__router.html</anchorfile>
      <anchor>a3b83e86d167da862000a1a4373ce6a03</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>EMBENET_BR_NetworkParameters</type>
      <name>NetworkParameters</name>
      <anchorfile>namespaceembenet_1_1border__router.html</anchorfile>
      <anchor>a78e8b6cfab5f935879fb13169b19babf</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>EMBENET_BR_NodeDetails</type>
      <name>NodeDetails</name>
      <anchorfile>namespaceembenet_1_1border__router.html</anchorfile>
      <anchor>a37a5fc2aaa14d82224b4d837902dc283</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>EMBENET_BR_AuthResult</type>
      <name>AuthResult</name>
      <anchorfile>namespaceembenet_1_1border__router.html</anchorfile>
      <anchor>ac6dee0a0582da40ec27acddea0413df6</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>errc</name>
      <anchorfile>namespaceembenet_1_1border__router.html</anchorfile>
      <anchor>a6c21be99529e163079c3e37572bec9a8</anchor>
      <arglist></arglist>
      <enumvalue file="namespaceembenet_1_1border__router.html" anchor="a6c21be99529e163079c3e37572bec9a8a334c4a4c42fdb79d7ebc3e73b517e6f8">none</enumvalue>
      <enumvalue file="namespaceembenet_1_1border__router.html" anchor="a6c21be99529e163079c3e37572bec9a8ae55d43eabeefe5a8271b4a3c898bd18f">invalid_argument</enumvalue>
      <enumvalue file="namespaceembenet_1_1border__router.html" anchor="a6c21be99529e163079c3e37572bec9a8aeb2b96e8967475b9eb5268929655a432">not_configured</enumvalue>
      <enumvalue file="namespaceembenet_1_1border__router.html" anchor="a6c21be99529e163079c3e37572bec9a8a7500611bf7030bc99d25c354e7b64714">not_found</enumvalue>
      <enumvalue file="namespaceembenet_1_1border__router.html" anchor="a6c21be99529e163079c3e37572bec9a8a7a6f43ded24333b44b8a5d5dc159c4ea">already_exists</enumvalue>
      <enumvalue file="namespaceembenet_1_1border__router.html" anchor="a6c21be99529e163079c3e37572bec9a8aafdb5594dc3e484fc1bfd7c564d550c1">not_enough_memory</enumvalue>
      <enumvalue file="namespaceembenet_1_1border__router.html" anchor="a6c21be99529e163079c3e37572bec9a8a618ead44397b73cf0baa08b460b66c39">no_buffer_space</enumvalue>
      <enumvalue file="namespaceembenet_1_1border__router.html" anchor="a6c21be99529e163079c3e37572bec9a8a20abf2ffbf6e82e36405843f269d6438">address_not_available</enumvalue>
      <enumvalue file="namespaceembenet_1_1border__router.html" anchor="a6c21be99529e163079c3e37572bec9a8a061a0de61b95c221bb4952c25e22f52d">no_root_node</enumvalue>
      <enumvalue file="namespaceembenet_1_1border__router.html" anchor="a6c21be99529e163079c3e37572bec9a8aaf169f73a82794128aa491c4b4f1c071">bad_address</enumvalue>
      <enumvalue file="namespaceembenet_1_1border__router.html" anchor="a6c21be99529e163079c3e37572bec9a8ada3dfa00a95010be4900c795a6d0e4fa">bad_socket</enumvalue>
      <enumvalue file="namespaceembenet_1_1border__router.html" anchor="a6c21be99529e163079c3e37572bec9a8add431dfaf61ea842693dca8be1ebcaf7">no_link</enumvalue>
      <enumvalue file="namespaceembenet_1_1border__router.html" anchor="a6c21be99529e163079c3e37572bec9a8a93b16d34b1689f66f0cfed60e99c0009">network_down</enumvalue>
      <enumvalue file="namespaceembenet_1_1border__router.html" anchor="a6c21be99529e163079c3e37572bec9a8a69c2dbb5917ca550a862e9c1c839bca1">not_connected</enumvalue>
      <enumvalue file="namespaceembenet_1_1border__router.html" anchor="a6c21be99529e163079c3e37572bec9a8ae048e2221fe3bee3dfab1ae6b95564a7">allocation_failure</enumvalue>
      <enumvalue file="namespaceembenet_1_1border__router.html" anchor="a6c21be99529e163079c3e37572bec9a8a88c7f07bc35dfa72d11bef8b2d6384c0">mutex_failure</enumvalue>
      <enumvalue file="namespaceembenet_1_1border__router.html" anchor="a6c21be99529e163079c3e37572bec9a8a1a96699cc53586c90ccd632b8a5cab70">semaphore_failure</enumvalue>
      <enumvalue file="namespaceembenet_1_1border__router.html" anchor="a6c21be99529e163079c3e37572bec9a8abc72ce356115238443d9bd536bdaac80">network_adapter_failure</enumvalue>
      <enumvalue file="namespaceembenet_1_1border__router.html" anchor="a6c21be99529e163079c3e37572bec9a8af55c0d0dab2c1db1893398016d016650">root_node_connector_failure</enumvalue>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>EventType</name>
      <anchorfile>namespaceembenet_1_1border__router.html</anchorfile>
      <anchor>a949fce2b3f5fad1b760f81b0324a3f95</anchor>
      <arglist></arglist>
      <enumvalue file="namespaceembenet_1_1border__router.html" anchor="a949fce2b3f5fad1b760f81b0324a3f95ac4e5afbb332c0b6053160f62794e71fc">network_started</enumvalue>
      <enumvalue file="namespaceembenet_1_1border__router.html" anchor="a949fce2b3f5fad1b760f81b0324a3f95a62b7ac05943a4f3a70a36b0495bcd896">network_stopped</enumvalue>
      <enumvalue file="namespaceembenet_1_1border__router.html" anchor="a949fce2b3f5fad1b760f81b0324a3f95a10c6bab9565035a4839cf01b947e6852">node_joined_network</enumvalue>
      <enumvalue file="namespaceembenet_1_1border__router.html" anchor="a949fce2b3f5fad1b760f81b0324a3f95a2fdb49b0277ec6fa475d884b694ce725">node_left_network</enumvalue>
      <enumvalue file="namespaceembenet_1_1border__router.html" anchor="a949fce2b3f5fad1b760f81b0324a3f95ac55824595b5b7e80ae4d68cbe2ce3952">node_joined_group</enumvalue>
      <enumvalue file="namespaceembenet_1_1border__router.html" anchor="a949fce2b3f5fad1b760f81b0324a3f95a2187e2e2f39d355ed0b668e74cc99dcb">node_left_group</enumvalue>
      <enumvalue file="namespaceembenet_1_1border__router.html" anchor="a949fce2b3f5fad1b760f81b0324a3f95a813bd59b2c7eadff4b8dbe51b64bdfa5">authentication_attempt</enumvalue>
      <enumvalue file="namespaceembenet_1_1border__router.html" anchor="a949fce2b3f5fad1b760f81b0324a3f95abcc9f493ca80067a7a8621e440ab3700">data_on_unregistered_port</enumvalue>
      <enumvalue file="namespaceembenet_1_1border__router.html" anchor="a949fce2b3f5fad1b760f81b0324a3f95aaf18ac1f4ae50b8540dc2c8727b5a6a3">packet_dropped</enumvalue>
      <enumvalue file="namespaceembenet_1_1border__router.html" anchor="a949fce2b3f5fad1b760f81b0324a3f95a599f6b665cac7d81e0913c204206c981">asn_update</enumvalue>
      <enumvalue file="namespaceembenet_1_1border__router.html" anchor="a949fce2b3f5fad1b760f81b0324a3f95a6dd8bc7549e4ab8ff518fe160bc5c0ef">root_node_connection_failed</enumvalue>
      <enumvalue file="namespaceembenet_1_1border__router.html" anchor="a949fce2b3f5fad1b760f81b0324a3f95a128995ca25b8df81ce9684fd5db5f5cd">root_node_identified</enumvalue>
    </member>
    <member kind="function">
      <type>char const *</type>
      <name>get_version_string</name>
      <anchorfile>namespaceembenet_1_1border__router.html</anchorfile>
      <anchor>a9516fa2aef6d6a60e07a7c4988a10538</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>set_border_router_udp_instance</name>
      <anchorfile>namespaceembenet_1_1border__router.html</anchorfile>
      <anchor>af2f36bb94abb62f0d2089a1e0c50795e</anchor>
      <arglist>(BorderRouter *instance)</arglist>
    </member>
    <member kind="function">
      <type>BorderRouter *</type>
      <name>get_border_router_udp_instance</name>
      <anchorfile>namespaceembenet_1_1border__router.html</anchorfile>
      <anchor>a04695dccff1e080ce421b1cbf7783358</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>std::error_category const &amp;</type>
      <name>border_router_error_category</name>
      <anchorfile>namespaceembenet_1_1border__router.html</anchorfile>
      <anchor>add6b6a5992c3a922a69839c81a45edcd</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>constexpr std::error_code</type>
      <name>make_error_code</name>
      <anchorfile>namespaceembenet_1_1border__router.html</anchorfile>
      <anchor>ac2449d85b938d0dddce3b65e597ce5be</anchor>
      <arglist>(errc error)</arglist>
    </member>
    <member kind="variable">
      <type>constexpr std::size_t</type>
      <name>event_alignment</name>
      <anchorfile>namespaceembenet_1_1border__router.html</anchorfile>
      <anchor>a1297c5c7ae19fdc48554b4340bea316d</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="namespace">
    <name>embenet::border_router::events</name>
    <filename>namespaceembenet_1_1border__router_1_1events.html</filename>
    <class kind="struct">embenet::border_router::events::RootNodeIdentified</class>
    <class kind="struct">embenet::border_router::events::NetworkStarted</class>
    <class kind="struct">embenet::border_router::events::NetworkStopped</class>
    <class kind="struct">embenet::border_router::events::NodeJoinedNetwork</class>
    <class kind="struct">embenet::border_router::events::NodeLeftNetwork</class>
    <class kind="struct">embenet::border_router::events::AuthenticationAttempt</class>
    <class kind="struct">embenet::border_router::events::NodeJoinedGroup</class>
    <class kind="struct">embenet::border_router::events::NodeLeftGroup</class>
    <class kind="struct">embenet::border_router::events::DataOnUnregisteredPort</class>
    <class kind="struct">embenet::border_router::events::PacketDropped</class>
    <class kind="struct">embenet::border_router::events::AsnUpdate</class>
    <class kind="struct">embenet::border_router::events::RootNodeConnectionFailed</class>
  </compound>
  <compound kind="group">
    <name>embenet</name>
    <title>Embenet</title>
    <filename>group__embenet.html</filename>
  </compound>
  <compound kind="group">
    <name>embenet_br</name>
    <title>embeNET Border Router API</title>
    <filename>group__embenet__br.html</filename>
  </compound>
  <compound kind="group">
    <name>embenet_border_router_c_api</name>
    <title>embeNET Border Router C API</title>
    <filename>group__embenet__border__router__c__api.html</filename>
  </compound>
  <compound kind="group">
    <name>embenet_common</name>
    <title>embeNET common definitions</title>
    <filename>group__embenet__common.html</filename>
    <class kind="struct">EMBENET_BR_CryptoKey</class>
    <class kind="struct">EMBENET_BR_EventHandlers</class>
    <class kind="struct">EMBENET_BR_NetworkParameters</class>
    <class kind="struct">EMBENET_BR_Capabilities</class>
    <class kind="struct">EMBENET_BR_JoinRule</class>
    <class kind="struct">EMBENET_BR_JoinPolicy</class>
    <class kind="struct">EMBENET_BR_NodesList</class>
    <class kind="struct">EMBENET_BR_GroupsList</class>
    <class kind="struct">EMBENET_BR_JoinRulesList</class>
    <class kind="struct">EMBENET_BR_NodeDetails</class>
  </compound>
  <compound kind="page">
    <name>md_embenet__brc__normative__references</name>
    <title>Normative References</title>
    <filename>md_embenet__brc__normative__references.html</filename>
  </compound>
  <compound kind="page">
    <name>embenet_brc_porting_guide</name>
    <title>embeNET BRC Porting Guide</title>
    <filename>embenet_brc_porting_guide.html</filename>
    <docanchor file="embenet_brc_porting_guide.html" title="embenet_brc_porting_guide.md">md_embenet__brc__porting__guide</docanchor>
  </compound>
  <compound kind="page">
    <name>embenet_brc_user_guide</name>
    <title>embeNET Border Router User Guide</title>
    <filename>embenet_brc_user_guide.html</filename>
    <docanchor file="embenet_brc_user_guide.html" title="embenet_brc_user_guide.md">md_embenet__brc__user__guide</docanchor>
    <docanchor file="embenet_brc_user_guide.html" title="Networking basics">embenet_brc_networking_basics</docanchor>
    <docanchor file="embenet_brc_user_guide.html" title="Overview of the network">embenet_brc_networking_basics_overview</docanchor>
    <docanchor file="embenet_brc_user_guide.html" title="Border router and nodes">embenet_brc_networking_border_router</docanchor>
    <docanchor file="embenet_brc_user_guide.html" title="Root node">embenet_brc_networking_root</docanchor>
    <docanchor file="embenet_brc_user_guide.html" title="IPv6 and multicast">embenet_brc_networking_ipv6</docanchor>
    <docanchor file="embenet_brc_user_guide.html" title="Starting the network">embenet_brc_networking_starting</docanchor>
    <docanchor file="embenet_brc_user_guide.html" title="Joining process and join rules">embenet_brc_networking_joining</docanchor>
    <docanchor file="embenet_brc_user_guide.html" title="UDP and sockets">embenet_brc_networking_udp</docanchor>
    <docanchor file="embenet_brc_user_guide.html" title="General information about the library">embenet_brc_architecture_general</docanchor>
    <docanchor file="embenet_brc_user_guide.html" title="Interfaces">embenet_brc_architecture_interfaces</docanchor>
    <docanchor file="embenet_brc_user_guide.html" title="Border Router C interface">embenet_brc_architecture_brc</docanchor>
    <docanchor file="embenet_brc_user_guide.html" title="UDP interface">embenet_brc_architecture_udp</docanchor>
    <docanchor file="embenet_brc_user_guide.html" title="Running the border router">embenet_brc_running</docanchor>
    <docanchor file="embenet_brc_user_guide.html" title="Exception handling">embenet_brc_stack_handling_exceptions</docanchor>
    <docanchor file="embenet_brc_user_guide.html" title="Starting the network">embenet_brc_network_start</docanchor>
    <docanchor file="embenet_brc_user_guide.html" title="Network configuration">embenet_brc_network_start_config</docanchor>
    <docanchor file="embenet_brc_user_guide.html" title="Example">embenet_brc_network_start_example</docanchor>
    <docanchor file="embenet_brc_user_guide.html" title="Stopping the network">embenet_brc_network_stop</docanchor>
    <docanchor file="embenet_brc_user_guide.html" title="Join rules management">embenet_brc_rules_management</docanchor>
    <docanchor file="embenet_brc_user_guide.html" title="Listing joined nodes">embenet_brc_listing_nodes</docanchor>
    <docanchor file="embenet_brc_user_guide.html" title="Listing joined groups">embenet_brc_listing_groups</docanchor>
    <docanchor file="embenet_brc_user_guide.html" title="Event handlers">embenet_brc_event_handlers</docanchor>
  </compound>
  <compound kind="dir">
    <name>embenet</name>
    <path>embenet/</path>
    <filename>dir_8af79a07e95c0b3929a81bd4a67f54dd.html</filename>
    <file>border_router.h</file>
    <file>border_router.hpp</file>
    <file>border_router_types.h</file>
    <file>border_router_types.hpp</file>
  </compound>
  <compound kind="dir">
    <name>embenet_port</name>
    <path>embenet_port/</path>
    <filename>dir_7707aec0a0f5afffebb8a67465a1a5f3.html</filename>
    <file>clock.h</file>
    <file>condition_variable.h</file>
    <file>memory.h</file>
    <file>mutex.h</file>
    <file>network_adapter.h</file>
    <file>random.h</file>
    <file>root_node_connector.h</file>
  </compound>
  <compound kind="page">
    <name>index</name>
    <title>embeNET Border Router C API</title>
    <filename>index.html</filename>
    <docanchor file="index.html" title="embeNET Border Router C API">mainpage</docanchor>
  </compound>
</tagfile>

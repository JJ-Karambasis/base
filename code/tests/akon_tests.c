UTEST(AKON, Test) {
	arena* Scratch = Scratch_Get();
	string TestContent = String_Lit(
								"Flags: { has_player_input: true } "
								"P: 0 0 5 "
								"BodyType: kinematic "
								"Shape: { Sphere: { Density: 5 } } "
								"Mesh: box "
								"Material: default ");
	akon_node_tree* NodeTree = AKON_Parse((allocator*)Scratch, TestContent);
	ASSERT_TRUE(NodeTree);
}
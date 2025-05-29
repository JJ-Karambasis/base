UTEST(AKON, Test) {
	string TestContent = String_Lit(
								"Flags: has_player_input "
								"P: 0 0 5 "
								"BodyType: kinematic "
								"Shape: { Sphere: { Density: 5 } } "
								"Mesh: box "
								"Material: default ");
	akon_node* Node = AKON_Parse(TestContent);
	ASSERT_TRUE(Node);
}


namespace front_end{
namespace parser{

class AstNode{

};
class AstStmt : public AstNode{

};
class AstIfStmt : public AstStmt {

};
class AstForStmt : public AstStmt {

};
class AstWhileStmt : public AstStmt {

};
class AstDoWhileStmt : public AstStmt {

};
class AstCompoundStmt : public AstStmt {

};
class AstDecl : public AstNode{

};
class AstExpression : public AstNode{

};
class AstBinaryExpr : public AstExpression{

};
class AstUnaryExpr : public AstExpression{

};
class AstFunction : public AstNode {

};
class AstFunctionProto : public AstNode {

};
class AstFunctionBody : public AstCompoundStmt {

};
class AstFunctionDecl : public AstDecl {

}; 

}//namespace parser
}// namespace front_end

open Virtual_dom.Vdom;

let view = id => Node.span(~attr=Attr.id(id), [Node.text("X")]);

```graphviz
digraph g {
    rankdir=LR
    node [margin=0.07, fontname="Arial", fixedsize=false, penwidth=3];
    edge [penwidth=1.5];

    wp [label=<wire<br/>protocol>, shape=circle, color=red, penwidth=5];

    subgraph cluster_0 {
        style=dotted;
        va [label=< voice<br/> adapter>, shape=box, style=rounded, color="#1a9988"];
        pd [label=<<b>pure data</b><br/>patch>; shape=cylinder, color="#eb5600"];
        libpd [label=<voices<br/>via <b>libpd</b>>, shape=circle, penwidth=5, color=green];
    }

    audio [label=<audio<br/>callback>, shape=circle, style=dashed, color=blue];

    wp -> va [color=red, penwidth=4];
    wp -> pd [color=red, penwidth=4];

    pd -> libpd [style=dashed, color="#eb5600"];
    va -> libpd [style=dashed, color="#1a9988"];
    
    //pd -> va [direction=none];

    libpd -> audio [penwidth=4, color=green];
}
```
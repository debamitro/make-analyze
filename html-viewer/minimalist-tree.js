function MinimalistTree (container)
{
    return {
        draw : (options) => expandOrCollapseNodes (container, options)
    }
}

function addNode (parent, node, options)
{
    var li = document.createElement ('li');

    li.appendChild (document.createTextNode (options.getlabel(node)));

    const children = options.getchildren (node);
    if (children != null && children.length > 0)
    {
        li.addEventListener ('click', (e) => {
            expandOrCollapseNodes (li, {
                getroots : () => children,
                getchildren : options.getchildren,
                getlabel : options.getlabel
            });
            e.stopPropagation ();
        });
    }
    else
    {
        li.addEventListener ('click', (e) => e.stopPropagation ());
    }

    parent.appendChild (li);
}

function expandOrCollapseNodes (parent, options)
{
    var existingUls = parent.getElementsByTagName ('ul');
    if (existingUls.length > 0)
    {
        // Toggle the state
        var styles = window.getComputedStyle (existingUls.item(0));
        if (styles.getPropertyValue ('display') == 'none')
        {
            existingUls.item(0).setAttribute ('style', 'display:block');
        }
        else
        {
            existingUls.item(0).setAttribute ('style', 'display:none');
        }
        return;
    }

    // Create a new list for the nodes
    var ul = document.createElement ('ul');
    options.getroots().forEach ( (node) => {
        addNode (ul, node, options);
    });
    parent.appendChild (ul);
}

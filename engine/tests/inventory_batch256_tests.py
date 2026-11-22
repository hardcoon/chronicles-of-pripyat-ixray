"""Replay actual cell/batch methods with resource-free UI adapters (MSVC x64).

This checks lifecycle and operation counts, not GPU time or in-game FPS.
Usage: python inventory_batch256_tests.py --out C:/.../tests --compiler C:/.../cl.exe
Run with the normal MSVC environment supplied by the build wrapper.
"""
from pathlib import Path
import argparse
import json
import subprocess


def extract(text, name):
    start = text.index(name)
    pos = text.index('{', start)
    end, depth = pos + 1, 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end]


def main():
    args = argparse.ArgumentParser()
    args.add_argument('--out', required=True, type=Path)
    args.add_argument('--compiler', required=True, type=Path)
    a = args.parse_args()
    source = Path(__file__).resolve().parent.parent / 'src/xrGame/ui'
    cell = (source / 'UICellItem.cpp').read_text(encoding='utf8')
    custom = (source / 'UICellCustomItems.cpp').read_text(encoding='utf8')
    lists = (source / 'UIDragDropListEx.cpp').read_text(encoding='utf8')
    methods = '\n'.join(extract(cell, n) for n in (
        'CUICellItem::CUICellItem()', 'CUICellItem::CUICellItem(bool',
        'void CUICellItem::EnsureWidgets()', 'void CUICellItem::PushChild(',
        'CUICellItem* CUICellItem::PopChild(', 'void CUICellItem::SetOwnerList(',
        'void CUICellItem::UpdateItemText()'))
    methods += '\n' + '\n'.join(extract(custom, n) for n in (
        'void CUIInventoryCellItem::UpdateItemText()',
        'u32 CUIAmmoCellItem::CalculateAmmoCount()',
        'void CUIAmmoCellItem::UpdateItemText()'))
    methods += '\n' + '\n'.join(extract(lists, n) for n in (
        'void CUIDragDropListEx::BeginItemBatch()', 'void CUIDragDropListEx::EndItemBatch()'))
    # Root placement must realize widgets before attaching/updating the cell.
    placement = extract(lists, 'void CUICellContainer::PlaceItemAtPos(')
    assert placement.index('itm->EnsureWidgets();') < placement.index('AttachChild')
    factory = (source / 'UICellItemFactory.cpp').read_text(encoding='utf8')
    assert 'new CUIWeaponCellItem(pWeapon)' in factory
    assert 'new CUIAmmoCellItem(pAmmo, deferWidgets)' in factory
    assert 'new CUIInventoryCellItem(itm, deferWidgets)' in factory
    code = r'''
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
using u32=unsigned;using string32=char[32];
#define R_ASSERT(x) do{if(!(x))std::abort();}while(0)
#define VERIFY(x) R_ASSERT(x)
#define xr_sprintf std::sprintf
unsigned assertions=0,widgetInitializations=0,helperScans=0,textUpdates=0;
void check(bool x){++assertions;if(!x)std::abort();}
struct CUIWindow{virtual ~CUIWindow()=default;};
struct CUIStatic{std::string text;bool shown=false;CUIStatic*TextItemControl(){return this;}void SetText(const char*s){text=s;++textUpdates;}void Show(bool b){shown=b;}};
struct CUIDragDropListEx;
struct Item{bool helper=false;u32 m_boxCurr=1;};
class CUICellItem:public CUIWindow{
public:
 std::vector<CUICellItem*>m_childs;CUIDragDropListEx*m_pParentList=nullptr;
 void*m_pData=nullptr,*m_custom_draw=nullptr,*m_custom_text=nullptr,*m_custom_mark=nullptr,*m_upgrade=nullptr,*m_pConditionState=nullptr;
 CUIStatic*m_text=nullptr;u32 m_drawn_frame=0;
 bool m_b_destroy_childs=true,m_selected=false,m_select_armament=false,m_select_equipped=false,m_cur_mark=false,m_has_upgrade=false,m_with_custom_text=false,m_with_custom_mark=false;
 CUICellItem();explicit CUICellItem(bool);~CUICellItem(){delete m_text;}
 void SetAccelerator(int){}void init(){++widgetInitializations;m_text=new CUIStatic;}
 void EnsureWidgets();virtual void UpdateItemText();void RefreshStackText(){UpdateItemText();}
 void PushChild(CUICellItem*);CUICellItem*PopChild(CUICellItem*);void SetOwnerList(CUIDragDropListEx*);
 u32 ChildsCount()const{return u32(m_childs.size());}
 virtual bool IsHelper(){++helperScans;return m_pData&&static_cast<Item*>(m_pData)->helper;}
};
struct CUIInventoryCellItem:CUICellItem{
 CUIInventoryCellItem(Item*i,bool defer=false):CUICellItem(defer){m_pData=i;}
 void UpdateItemText()override;
};
struct CUIAmmoCellItem:CUIInventoryCellItem{
 using CUIInventoryCellItem::CUIInventoryCellItem;Item*object(){return static_cast<Item*>(m_pData);}
 u32 CalculateAmmoCount();void UpdateItemText()override;
};
namespace detail{struct is_helper_pred{bool operator()(CUICellItem*c){return c->IsHelper();}};}
struct Container{int csUi=0;std::vector<CUIWindow*>roots;auto&GetChildWndList(){return roots;}};
struct xrCriticalSectionGuard{explicit xrCriticalSectionGuard(int&) {}};
struct CUIDragDropListEx{bool m_itemBatch=false;Container container;Container*m_container=&container;
 void BeginItemBatch();void EndItemBatch();bool IsItemBatch()const{return m_itemBatch;}
};
METHODS
int main(){
 {CUICellItem eager;check(eager.m_text!=nullptr);auto n=widgetInitializations;eager.EnsureWidgets();check(widgetInitializations==n);}
 {CUICellItem lazy(true);check(lazy.m_text==nullptr);lazy.RefreshStackText();check(lazy.m_text==nullptr);lazy.EnsureWidgets();check(lazy.m_text!=nullptr);}
 unsigned oldWidgets=widgetInitializations,oldScans=helperScans;
 std::vector<Item>items(1001);for(auto&i:items)i.m_boxCurr=7;
 std::vector<std::unique_ptr<CUIAmmoCellItem>>cells;
 CUIDragDropListEx list;
 for(auto&i:items)cells.push_back(std::make_unique<CUIAmmoCellItem>(&i,true));
 check(widgetInitializations==oldWidgets);
 auto*root=cells[0].get();root->EnsureWidgets();root->SetOwnerList(&list);list.container.roots.push_back(root);
 auto before=textUpdates;list.BeginItemBatch();
 for(unsigned n=1;n<cells.size();++n){root->PushChild(cells[n].get());cells[n]->SetOwnerList(&list);}
 check(textUpdates==before);check(root->ChildsCount()==1000);
 list.EndItemBatch();check(!list.IsItemBatch());check(root->m_text->text=="7007");
 check(widgetInitializations-oldWidgets==1);check(helperScans-oldScans==1002);
 check(textUpdates-before==1);
 // PopChild swaps live item data; the detached child can become a visible root.
 Item*rootData=static_cast<Item*>(root->m_pData);auto*detached=root->PopChild(nullptr);
 check(detached->m_pData==rootData&&detached->m_pParentList==nullptr&&detached->m_text==nullptr);
 detached->EnsureWidgets();check(detached->m_text->text=="7"&&widgetInitializations-oldWidgets==2);
 detached->EnsureWidgets();check(widgetInitializations-oldWidgets==2);
 // Later insertion outside a batch preserves immediate native counter updates.
 root->PushChild(detached);check(root->m_text->text=="7007");
 // Specific-child removal preserves that child's data even when back() differs.
 auto*needed=cells[1].get();void*neededData=needed->m_pData;auto*removed=root->PopChild(needed);
 check(removed->m_pData==neededData&&removed->ChildsCount()==0&&removed->m_pParentList==nullptr);
 // Helper ammo must not enter the total; regular inventory labels also refresh.
 items[2].helper=true;list.BeginItemBatch();list.EndItemBatch();
 check(root->m_text->text=="6993");
 Item a,b;CUIInventoryCellItem generic(&a,true),child(&b,true);
 generic.EnsureWidgets();generic.SetOwnerList(&list);list.container.roots={&generic};
 list.BeginItemBatch();generic.PushChild(&child);list.EndItemBatch();
 check(generic.m_text->text=="x2"&&child.m_text==nullptr);
 auto*promoted=generic.PopChild(nullptr);promoted->EnsureWidgets();check(promoted->m_text!=nullptr&&!promoted->m_text->shown);
 std::cout<<"PASS: "<<assertions<<" exact-source batch/lazy-widget/ammo/promotion assertions; 1001 physical objects -> 1 widget initialization, 1 batch counter update\n";
}
'''.replace('METHODS', methods)
    a.out.mkdir(parents=True, exist_ok=True)
    cpp = a.out / 'inventory_batch256.cpp'
    cpp.write_text(code, encoding='utf8')
    log = a.out / 'inventory_batch256.log'
    with log.open('wb') as output:
        subprocess.run([str(a.compiler), '/nologo', '/std:c++20', '/EHsc', '/O2', '/MD',
                        str(cpp), '/Fo:' + str(a.out / 'inventory_batch256.obj'),
                        '/Fe:' + str(a.out / 'inventory_batch256.exe')],
                       stdout=output, stderr=subprocess.STDOUT, check=True)
        subprocess.run([str(a.out / 'inventory_batch256.exe')], stdout=output,
                       stderr=subprocess.STDOUT, check=True)
    result = dict(success=True, scope='Exact production methods with resource-free adapters; no runtime timing claim',
                  log=str(log), result=log.read_text(errors='replace').strip())
    (a.out / 'inventory_batch256.json').write_text(json.dumps(result, indent=2), encoding='utf8')
    print(result['result'])


if __name__ == '__main__':
    main()

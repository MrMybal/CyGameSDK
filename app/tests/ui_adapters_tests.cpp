#include "UiPropertyAdapter.h"
#include <cyui/modules/DataView.h>
#include <iostream>
using namespace cygame;
namespace {
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
const PropertyCaption captions=[](const auto&,const auto& value){return value;};
void checkRoundTrip(const Json& schema,const Json& value){
    const auto property=uiPropertyValue(schema,value,"fixture",captions);
    require(jsonPropertyValue(schema,property,value)==value,"Typed UI conversion changed authored data");
}
}
int main(){
    try{
        const Json integer={{"type","integer"},{"minimum",INT64_MIN},{"maximum",INT64_MAX}};
        checkRoundTrip(integer,Json(int64_t(9007199254740993LL)));
        checkRoundTrip({{"type","array"},{"items",integer},{"x-editor-container","array.int"}},Json::array({int64_t(9007199254740993LL),-7}));
        const Json array={{"type","array"},{"items",{{"type","number"},{"minimum",-10},{"maximum",10}}},{"x-editor-container","array.float"}};
        auto empty=uiPropertyValue(array,Json::array(),"fixture",captions);
        require(std::holds_alternative<cyui::PropertyArray>(empty),"Empty typed array lost its kind");
        require(std::get<cyui::PropertyArray>(empty).elementTemplate!=nullptr,"Empty array lost its element schema");
        require(std::get<double>(*std::get<cyui::PropertyArray>(empty).elementTemplate)==0.,"New array values must use zero within bounds");
        checkRoundTrip({{"type","array"},{"items",integer},{"uniqueItems",true},{"x-editor-container","set.int"}},Json::array({7,9}));
        checkRoundTrip({{"type","object"},{"additionalProperties",integer}},{{"A",int64_t(9007199254740993LL)},{"B",-2}});
        const Json enumeration={{"type","string"},{"enum",{"None","Default","##authored"}},{"x-editor-nominalType","enum.Mode"}};
        checkRoundTrip(enumeration,"Default");
        cyui::Property from,to;from.id=1;to.id=2;from.value=uiPropertyValue(enumeration,"Default","graph-A",captions);to.value=uiPropertyValue(enumeration,"Default","graph-B",captions);
        require(!cyui::propertyFromJson(to,cyui::propertyToJson(from)),"Cross-graph nominal clipboard was accepted");
        const Json record={{"type","object"},{"x-editor-nominalType","struct.Spawn"},{"properties",{{"id",integer},{"mode",enumeration}}}};
        checkRoundTrip(record,{{"id",int64_t(9007199254740993LL)},{"mode","None"},{"vendorExtra",{{"keep",true}}}});
        const Json rgb={{"type","array"},{"x-editor-widget","color"},{"minItems",3},{"maxItems",3}};
        checkRoundTrip(rgb,Json::array({.1,.2,.3}));
        // Typed TSV must reject a partial batch rather than leaving the first valid cell applied.
        std::vector<cyui::DataColumn> columns(2);columns[0].id=1;columns[1].id=2;columns[0].editable=columns[1].editable=true;
        columns[1].metadata.readOnly=false;
        std::vector<cyui::DataRow> rows{{10,{int64_t(1),int64_t(2)},false}};
        cyui::DataViewState selection;selection.selection={10};selection.anchor=selection.active=10;selection.anchorColumn=0;selection.activeColumn=1;
        auto valid=cyui::pasteDataCells(columns,rows,selection,"3\t4");
        require(valid.edits.size()==2,"Typed TSV fixture must propose both cells");
        columns[1].metadata.readOnly=true;
        auto paste=cyui::pasteDataCells(columns,rows,selection,"3\t4");
        require(!paste&&paste.edits.empty(),"Read-only TSV paste did not reject the whole batch");
        require(std::get<int64_t>(rows[0].cells[0])==1,"TSV proposal mutated the host model");
        std::cout<<"CyUI adapter contracts: PASS\\n";return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\\n';return 1;}
}

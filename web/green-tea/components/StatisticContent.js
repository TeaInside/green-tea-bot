import GraphStatistic from "./GraphStatistic"
import TableStatistic from "./TableStatistic"

export default function StatisticContent(){
    return (
        <div className="w-full h-screen bg-blue-800">
            <h1 className="text-center text-[6em]">STATISTIC</h1>

            <div className="grid grid-cols-3 grid-rows-2 h-[800px] mx-16 mt- gap-x-10 mb-10">
                <GraphStatistic />
                <TableStatistic />
            </div>
        </div>
        
    )
}
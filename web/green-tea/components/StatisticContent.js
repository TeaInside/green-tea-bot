import GraphStatistic from "./GraphStatistic";
import TableStatistic from "./TableStatistic";

export default function StatisticContent() {
    return (
        <div className="w-full h-screen bg-cream overflow-y-scroll whitespace-nowrap py-10">
            <h1 className="text-center  text-[3em]">STATISTIC</h1>

            <div className="grid grid-cols-3 grid-rows-2 mx-16 mt-8 gap-x-10">
                <GraphStatistic />
                <TableStatistic />
            </div>
        </div>
    );
}

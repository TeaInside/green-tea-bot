import WordStatisticRow from "./WordStatisticRow";

export default function TableWordStatistic() {
    return (
        <div className="bg-violet-900 row-span-1 overflow-y-scroll flex-1">
            <h1 className="text-center text-white text-[2em]">Top 20 Word Cloud</h1>
            <h1 className="text-center text-white text-[1.5em]">Koding Teh</h1>
            <table className="w-10/12 mx-auto text-center border-solid bg-white border-2 mt-4">
                <thead className="border-solid bg-white border-2">
                    <tr>
                        <th className=" border-solid bg-white border-2 w-0">No.</th>
                        <th className=" border-solid bg-white border-2 w-24">Word</th>
                        <th className=" border-solid bg-white border-2 w-24">Occurences</th>
                    </tr>
                </thead>
                <tbody className="border-solid bg-white border-1">
                    <WordStatisticRow />
                    <WordStatisticRow />
                    <WordStatisticRow />
                    <WordStatisticRow />
                    <WordStatisticRow />
                    <WordStatisticRow />
                    <WordStatisticRow />
                    <WordStatisticRow />
                </tbody>
            </table>
        </div>
    );
}

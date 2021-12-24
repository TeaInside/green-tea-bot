import WordStatisticRow from "./WordStatisticRow"

export default function TableWordStatistic() {
    return(
        <div className="bg-purple-500 row-span-1 overflow-y-scroll">
            <h1 className="text-center bg-green-800 text-[30px]">Top 20 Word Cloud</h1>
            <h1 className="text-center">Koding Teh</h1>
            <table className="w-10/12 mx-auto text-center border-solid border-2">
                <thead className="border-solid border-2">
                    <tr>
                        <th className=" border-solid border-2 w-0">No.</th>
                        <th className=" border-solid border-2 w-24">Word</th>
                        <th className=" border-solid border-2 w-24">Occurences</th>
                    </tr>
                </thead>
                <tbody className="border-solid border-1">
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
    )
}
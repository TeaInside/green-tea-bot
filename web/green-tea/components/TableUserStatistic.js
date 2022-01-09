import UserStatisticRow from "./UserStatisticRow";

export default function TableUserStatistic() {
    return (
        <div className="bg-pink-800 overflow-y-scroll row-span-1 flex-1 py-4">
            <h1 className="text-center text-white text-[2em]">User Stats</h1>
            <h1 className="text-center text-white text-[1.5em]">Koding Teh</h1>
            <table className="w-10/12 mx-auto text-center bg-white border-2 mt-4">
                <thead className="">
                    <tr>
                        <th className="w-0 border-2 ">No.</th>
                        <th className="w-24 border-2">Photo</th>
                        <th className="w-32 border-2">Name</th>
                        <th className="w-24 border-2">Messages</th>
                    </tr>
                </thead>
                <tbody className="">
                    <UserStatisticRow />
                    <UserStatisticRow />
                    <UserStatisticRow />
                    <UserStatisticRow />
                    <UserStatisticRow />
                    <UserStatisticRow />
                    <UserStatisticRow />
                </tbody>
            </table>
        </div>
    );
}
